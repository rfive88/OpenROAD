# Spike Brief S0 — ipl Skeleton: Build, Test, and Commit

## Goal

The ipl skeleton source files were already written in a prior session (see
"Files Already Present" below). This brief's job is to add the missing Bazel
build rules, wire ipl into the top-level `BUILD.bazel`, verify both the Bazel
and CMake builds are clean, run the `hello_ipl` smoke test, and land
everything on the `ipl` branch in a single signed commit.

## Context

- The active git branch is `ipl`. Confirm with `git branch --show-current`
  before touching anything.
- There may be a stale `.git/index.lock` from a prior session. Remove it
  before any git operation: `rm -f ~/projects/OpenROAD/.git/index.lock`
- OpenROAD uses **two parallel build systems**: Bazel (primary, native macOS
  binary) and CMake (CI). Both must be kept in sync. The CMake files were
  already written; this brief adds the Bazel files.
- Do **not** modify anything under `src/sta/`. Do not touch files in modules
  other than those listed in "Files to Modify" below.
- `utl::Logger::report()` is the right call for the `hello_ipl` message — it
  emits at default verbosity with no tool-id prefix, matching the `.ok` file.
- Bazel build target for the full binary: `bazel build //:openroad`
- Bazel test invocation: `bazel test //src/ipl/test:hello_ipl`
- All source files already written use the `ipl` namespace throughout.

## Files Already Present (do not recreate)

```
src/ipl/
├── CMakeLists.txt
├── README.md
├── docs/briefs/               ← this brief lives here
├── include/ipl/
│   ├── IntegratedPlacer.h
│   └── MakeIntegratedPlacer.h
├── src/
│   ├── IntegratedPlacer.cpp
│   ├── MakeIntegratedPlacer.cpp
│   ├── ipl.i
│   └── ipl.tcl
└── test/
    ├── CMakeLists.txt
    ├── hello_ipl.ok
    └── hello_ipl.tcl
```

Modified OpenROAD infrastructure files (already done):
- `include/ord/OpenRoad.hh` — `ipl::IntegratedPlacer*` forward-decl, member,
  `getIntegratedPlacer()` getter
- `src/OpenRoad.cc` — includes, `new ipl::IntegratedPlacer(db_, logger_)`,
  destructor `delete integrated_placer_`, `ipl::initIntegratedPlacer(tcl_interp)`
- `src/OpenRoad.i` — `#include "ipl/IntegratedPlacer.h"`, `getIntegratedPlacer()`
  accessor function
- `src/CMakeLists.txt` — `add_subdirectory(ipl)`, `ipl` in link libs
- `src/utl/include/utl/Logger.h` — `X(IPL)` added to the tool-id X-macro

## Files to Create

### `src/ipl/BUILD`

```python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# ipl: core placer logic — no GUI dependencies.
# C++ unit tests link against this target directly to avoid pulling in
# the full OpenROAD GUI stack.
cc_library(
    name = "ipl",
    srcs = [
        "src/IntegratedPlacer.cpp",
    ],
    hdrs = [
        "include/ipl/IntegratedPlacer.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb/src/db",
        "//src/utl",
    ],
)

# ui: Tcl/SWIG registration layer.  OpenROAD's top-level BUILD.bazel
# depends on this target (as //src/ipl:ui).
cc_library(
    name = "ui",
    srcs = [
        "src/MakeIntegratedPlacer.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/ipl/MakeIntegratedPlacer.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":ipl",
        "//:ord",
        "//src/utl",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/ipl.tcl",
    ],
    char_array_name = "ipl_tcl_inits",
    namespace = "ipl",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/ipl.i",
        "//:error_swig",
    ],
    module = "ipl",
    namespace_prefix = "ipl",
    root_swig_src = "src/ipl.i",
    swig_includes = [
        "src/ipl/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
```

### `src/ipl/test/BUILD`

```python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "hello_ipl",
]

filegroup(
    name = "test_resources",
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]
```

### `src/ipl/docs/briefs/README.md`

```markdown
# src/ipl/docs/briefs/

Spike briefs for the Integrated Placer (ipl). Each brief specifies one
implementation stage: goal, context, exact files to create/modify, API,
implementation notes, tests, and a hard gate that must pass before the
next brief begins.

| Brief | Stage | Description |
|-------|-------|-------------|
| spike-ipl-S0-skeleton.md | S0 | ipl skeleton: BUILD, hello_ipl, commit |
```

## Files to Modify

### `BUILD.bazel` (top-level)

Find the block containing `"//src/gpl"` and `"//src/gpl:ui"` and add the
`ipl` lines immediately after, keeping alphabetical order with the `i` tools:

```python
    # existing lines around insertion point:
    "//src/gpl",
    "//src/gpl:ui",
    # ADD these two lines:
    "//src/ipl",
    "//src/ipl:ui",
    # existing continuation:
    "//src/grt",
```

There is a second occurrence of tool targets further down in `BUILD.bazel`
(around line 387). Search for `"//src/gpl"` and add `"//src/ipl"` in both
locations where the other tools appear.

## Build Verification

### Bazel (primary — native macOS binary)

```bash
cd ~/projects/OpenROAD

# Full binary build — confirms ipl links cleanly into OpenROAD
bazel build //:openroad

# ipl Tcl smoke test
bazel test //src/ipl/test:hello_ipl --test_output=all
```

Expected test output:
```
hello_ipl
```

### CMake (secondary — keep in sync)

```bash
cd ~/projects/OpenROAD
cmake -B build -DENABLE_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build -R hello_ipl --output-on-failure
```

If the cmake build tree does not exist yet, the `-B build` step will
configure it from scratch (this is expected on a fresh checkout).

## Commit

Once both builds are clean and the test passes:

```bash
cd ~/projects/OpenROAD
rm -f .git/index.lock          # clear stale lock if present
git add src/ipl/ \
        include/ord/OpenRoad.hh \
        src/CMakeLists.txt \
        src/OpenRoad.cc \
        src/OpenRoad.i \
        src/utl/include/utl/Logger.h \
        BUILD.bazel
git status                     # confirm only expected files are staged
git commit -s -m "ipl: S0 skeleton — hello_ipl Tcl command wired into OpenROAD

Adds the ipl (Integrated Placer) module skeleton:
- src/ipl/{include,src,test,docs/} directory structure
- IntegratedPlacer class with hello_ipl() diagnostic command
- SWIG interface (ipl.i) and Tcl wrapper (ipl.tcl)
- Bazel BUILD and CMakeLists.txt build rules
- hello_ipl integration test (.tcl + .ok)
- X(IPL) tool-id in utl/Logger.h
- Wired into OpenRoad.hh / OpenRoad.cc / OpenRoad.i
- add_subdirectory(ipl) and //src/ipl:ui in top-level build files

No functional placement logic yet. Subsequent briefs in
src/ipl/docs/briefs/ will add placement stages one at a time."
```

## Deliverables Checklist

- [ ] `src/ipl/BUILD` created with `ipl`, `ui`, `tcl`, `swig` targets
- [ ] `src/ipl/test/BUILD` created with `regression_test` for `hello_ipl`
- [ ] `src/ipl/docs/briefs/README.md` created
- [ ] `BUILD.bazel` updated with `//src/ipl` and `//src/ipl:ui` in both locations
- [ ] `bazel build //:openroad` succeeds with no errors or new warnings
- [ ] `bazel test //src/ipl/test:hello_ipl` passes (output matches `hello_ipl`)
- [ ] `cmake` + `ctest` also green for `hello_ipl`
- [ ] `git commit -s` on branch `ipl` with the message above

## Hard Gate

Do not proceed to S1 (first placement brief) until:
1. `bazel test //src/ipl/test:hello_ipl` reports PASSED
2. `git log --oneline -1` shows the S0 commit on branch `ipl`
3. `git status` is clean (nothing unstaged or untracked in `src/ipl/`)
