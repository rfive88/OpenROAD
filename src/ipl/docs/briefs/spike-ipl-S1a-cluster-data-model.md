# Spike Brief S1a — Cluster Data Model

## Goal

Implement the pure C++ cluster data model for ipl's physical hierarchy:
`Metrics`, `Cluster`, and `PhysicalHierarchy`.  No OpenDB traversal, no
Tcl commands, no placement logic.  This layer is the foundation that S1b
(hierarchy extraction), S1c (autocluster refinement), S2 (macro placement),
and S3 (analytical cell placement) all build on.

The key architectural requirement is that this layer is **reusable across
both the design-planning flow (S1+S2) and the full integrated placement flow
(S1+S2+S3)**.  `ClusteringEngine` (S1b) produces a `PhysicalHierarchy`;
`MacroPlacer` (S2) and `AnalyticalPlacer` (S3) consume it.  Nothing in this
layer knows about placement or DB traversal.

## Reference

- Hier-RTLMP paper (§III-A: Cluster Definition, §III-B: Cluster Metrics)
  — `~/projects/eda-lab/docs/papers/02_Hier-RTLMP_2023.pdf`
- `src/mpl/src/object.h` in this repo — read for algorithm understanding
  only; do **not** copy any code.  ipl's implementation must be written
  fresh.

## Context

- Active branch: `ipl`.  Confirm with `git branch --show-current`.
- All new files go under `src/ipl/src/`.
- `src/ipl/include/ipl/IntegratedPlacer.h` already exists — do not modify
  it in this brief.
- `src/ipl/CMakeLists.txt` and `src/ipl/BUILD` exist — both must be updated
  to compile the three new `.cpp` files.
- Follow OpenROAD code style: `snake_case` for methods and members, `CamelCase`
  for classes.  All member variables use a trailing `_` (e.g. `num_macro_`).
  `#pragma once`, BSD-3 license header, `clang-format` before commit.
- Use `utl::Logger` for any runtime messages.  Message id range for ipl:
  1–199 (`utl::IPL`).  S1a has no mandatory log messages; the logger pointer
  is threaded through for future use.
- Do **not** add `par::PartitionMgr` as a dependency in S1a — it is needed
  only by `ClusteringEngine` (S1c) for large-cluster breaking.

## Architecture

```
src/ipl/src/
├── metrics.h / metrics.cpp          (NEW — S1a)
├── cluster.h / cluster.cpp          (NEW — S1a)
├── physical_hierarchy.h / .cpp      (NEW — S1a)
├── cluster_engine.h / .cpp          (future S1b/S1c)
├── IntegratedPlacer.h / .cpp        (exists — S0)
└── MakeIntegratedPlacer.cpp         (exists — S0)
```

`PhysicalHierarchy` owns the cluster tree (via `root`), the threshold
parameters that drive `ClusteringEngine`, and the id/inst lookup maps needed
by later stages.

## Files to Create

### `src/ipl/src/metrics.h`

```cpp
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Metrics aggregates instance-count and area statistics for a logical
// module or a physical cluster.  It is used by ClusteringEngine to
// decide when to merge or split clusters (see Hier-RTLMP §III-B).
// All areas are in OpenDB database units squared (dbu²).

#pragma once
#include <cstdint>
#include <utility>

namespace ipl {

class Metrics
{
 public:
  Metrics() = default;
  Metrics(unsigned int num_std_cell,
          unsigned int num_macro,
          int64_t std_cell_area,
          int64_t macro_area);

  // Accumulate another Metrics into this one (used when merging clusters).
  void add(const Metrics& other);

  unsigned int getNumStdCell() const { return num_std_cell_; }
  unsigned int getNumMacro()   const { return num_macro_; }
  int64_t getStdCellArea()     const { return std_cell_area_; }
  int64_t getMacroArea()       const { return macro_area_; }
  int64_t getTotalArea()       const { return std_cell_area_ + macro_area_; }

  bool empty() const { return num_std_cell_ == 0 && num_macro_ == 0; }

 private:
  unsigned int num_std_cell_{0};
  unsigned int num_macro_{0};
  int64_t std_cell_area_{0};   // dbu²
  int64_t macro_area_{0};      // dbu²
};

}  // namespace ipl
```

### `src/ipl/src/metrics.cpp`

Implement `Metrics(...)` constructor and `add()`.  Simple member-wise
construction and accumulation — no logic beyond that.

### `src/ipl/src/cluster.h`

```cpp
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Cluster is a node in the ipl physical hierarchy tree.
//
// The tree is built by ClusteringEngine (S1b/S1c) from the OpenDB module
// hierarchy.  Each cluster holds:
//   - its type (HardMacroCluster | StdCellCluster | MixedCluster | IOCluster)
//   - references to the leaf dbInst* members classified into this cluster
//   - references to the dbModule* logical modules that map to it
//   - aggregated Metrics (instance counts + areas)
//   - tree links: one parent, zero or more owned children
//   - inter-cluster connection weights (used by SA macro placer in S2)
//
// Ownership: each Cluster owns its children via unique_ptr.
// The root is owned by PhysicalHierarchy.
//
// Physical coordinates (x, y, width, height) are populated by the macro
// placer (S2) and analytical placer (S3); they are zero-initialised here.

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "metrics.h"
#include "odb/db.h"

namespace utl {
class Logger;
}

namespace ipl {

// ClusterType mirrors the classification used in Hier-RTLMP §III-A.
// IOCluster is added for ipl to represent bundled I/O pin groups.
enum ClusterType
{
  HardMacroCluster,  // leaf cluster containing only hard macros
  StdCellCluster,    // leaf cluster containing only standard cells
  MixedCluster,      // internal cluster with both macros and std cells
  IOCluster          // bundled I/O pin group on the die boundary
};

// ConnectionsMap: cluster_id → aggregated net-crossing connection weight.
// Populated by ClusteringEngine::buildNetListConnections() in S1b.
using ConnectionsMap = std::map<int, float>;
using UniqueClusterVector = std::vector<std::unique_ptr<Cluster>>;

class Cluster
{
 public:
  Cluster(int id, std::string name, utl::Logger* logger);

  // Identity
  int getId()   const { return id_; }
  const std::string& getName() const { return name_; }
  void setName(const std::string& name) { name_ = name; }

  // Type
  ClusterType getType() const { return type_; }
  void setType(ClusterType type) { type_ = type; }
  std::string getTypeString() const;

  // Metrics
  const Metrics& getMetrics() const { return metrics_; }
  void setMetrics(const Metrics& m) { metrics_ = m; }
  unsigned int getNumStdCell() const { return metrics_.getNumStdCell(); }
  unsigned int getNumMacro()   const { return metrics_.getNumMacro(); }
  int64_t getTotalArea()       const { return metrics_.getTotalArea(); }

  // Logical module associations (one cluster may span multiple dbModules)
  void addDbModule(odb::dbModule* module);
  const std::vector<odb::dbModule*>& getDbModules() const;
  void clearDbModules();

  // Leaf instance membership
  void addLeafStdCell(odb::dbInst* inst);
  void addLeafMacro(odb::dbInst* inst);
  const std::vector<odb::dbInst*>& getLeafStdCells() const;
  const std::vector<odb::dbInst*>& getLeafMacros()   const;
  void clearLeafInstances();

  // Tree links
  Cluster* getParent() const { return parent_; }
  void setParent(Cluster* parent) { parent_ = parent; }

  void addChild(std::unique_ptr<Cluster> child);
  const UniqueClusterVector& getChildren() const { return children_; }
  std::unique_ptr<Cluster> releaseChild(const Cluster* target);
  UniqueClusterVector releaseChildren();
  bool isLeaf() const { return children_.empty(); }

  // Physical position (populated in S2/S3; zero until then)
  int getX() const { return x_; }
  int getY() const { return y_; }
  int getWidth()  const { return width_; }
  int getHeight() const { return height_; }
  void setLocation(int x, int y) { x_ = x; y_ = y; }
  void setDimensions(int width, int height) { width_ = width; height_ = height; }

  // Inter-cluster connections (net-crossing weights; built in S1b)
  void addConnection(int other_cluster_id, float weight);
  void removeConnection(int other_cluster_id);
  const ConnectionsMap& getConnectionsMap() const { return connections_; }
  void clearConnections() { connections_.clear(); }

  // Debug
  void printTree(int indent = 0) const;

 private:
  int id_{-1};
  std::string name_;
  ClusterType type_{MixedCluster};
  Metrics metrics_;

  std::vector<odb::dbModule*> db_modules_;
  std::vector<odb::dbInst*> leaf_std_cells_;
  std::vector<odb::dbInst*> leaf_macros_;

  // Physical coords (dbu); zero until populated by placement stages
  int x_{0};
  int y_{0};
  int width_{0};
  int height_{0};

  Cluster* parent_{nullptr};          // raw, non-owning
  UniqueClusterVector children_;      // owned

  ConnectionsMap connections_;        // net-crossing weights

  utl::Logger* logger_{nullptr};      // non-owning
};

}  // namespace ipl
```

### `src/ipl/src/cluster.cpp`

Implement all methods declared above.  Key points:
- `addChild()`: set `child->setParent(this)` before `push_back`
- `releaseChild()`: find by pointer comparison, move out of `children_`,
  null the parent on the released cluster, return it
- `releaseChildren()`: move the entire `children_` vector out, null
  the parent pointer on each released child, return
- `getTypeString()`: return `"HardMacro"`, `"StdCell"`, `"Mixed"`, or `"IO"`
- `addConnection()`: accumulate weight (not replace) if key already exists
- `printTree()`: indent by `level * 2` spaces, print `[type] name  (macros=N cells=M)`

### `src/ipl/src/physical_hierarchy.h`

```cpp
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// PhysicalHierarchy is the top-level container for the ipl cluster tree.
//
// It is produced by ClusteringEngine (S1b/S1c) and consumed by:
//   - MacroPlacer (S2): reads the tree, assigns macro coordinates
//   - AnalyticalPlacer (S3): reads cluster membership, drives density
//
// This struct holds three categories of data:
//   1. The cluster tree (root + lookup maps)
//   2. Threshold parameters that control how ClusteringEngine builds the tree
//   3. Design-level flags and geometry (populated by ClusteringEngine)

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "cluster.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace ipl {

// Lookup maps from OpenDB objects to cluster ids (populated by
// ClusteringEngine; used by placement stages for quick lookup).
struct PhysicalHierarchyMaps
{
  // cluster id → Cluster* (non-owning, for O(1) id lookup)
  std::unordered_map<int, Cluster*> id_to_cluster;
  // dbInst* → cluster id (leaf instance assignment)
  std::unordered_map<odb::dbInst*, int> inst_to_cluster_id;
  // dbBTerm* → cluster id (I/O pin assignment; populated in S1b)
  std::unordered_map<odb::dbBTerm*, int> bterm_to_cluster_id;
};

struct PhysicalHierarchy
{
  // -----------------------------------------------------------------------
  // Cluster tree
  // -----------------------------------------------------------------------
  std::unique_ptr<Cluster> root;   // owns the entire tree
  PhysicalHierarchyMaps maps;

  // -----------------------------------------------------------------------
  // Clustering threshold parameters
  // These are set by the caller (IntegratedPlacer) before handing the
  // struct to ClusteringEngine.  ClusteringEngine will compute per-level
  // derived limits (max_macro_, min_macro_, etc.) from these base values.
  // See Hier-RTLMP §III-C for the derivation.
  // -----------------------------------------------------------------------
  int base_max_macro{0};         // max hard macros per cluster at level 0
  int base_min_macro{0};         // min hard macros per cluster at level 0
  int base_max_std_cell{0};      // max std cells per cluster at level 0
  int base_min_std_cell{0};      // min std cells per cluster at level 0
  int max_num_level{0};          // max hierarchy depth to generate
  int large_net_threshold{0};    // nets with more pins than this are ignored
  float cluster_size_ratio{0.0f};      // per-level size scaling ratio
  float cluster_size_tolerance{0.0f};  // merge tolerance fraction

  // -----------------------------------------------------------------------
  // Design-level data (populated by ClusteringEngine in S1b)
  // -----------------------------------------------------------------------
  odb::Rect die_area;            // from dbBlock::getDieArea()
  odb::Rect floorplan_shape;     // usable core area (may differ from die)
  int64_t macro_with_halo_area{0};  // sum of macro areas including halos

  // Design composition flags (set by ClusteringEngine in S1b)
  bool has_io_clusters{true};
  bool has_only_macros{false};
  bool has_std_cells{true};
  bool has_unfixed_macros{true};
  bool has_fixed_macros{false};

  // -----------------------------------------------------------------------
  // Helpers
  // -----------------------------------------------------------------------

  // Return the Cluster* for a given id (nullptr if not found).
  Cluster* findCluster(int id) const;

  // Return the cluster id that contains inst (-1 if not assigned).
  int findClusterId(odb::dbInst* inst) const;

  // Register a newly created cluster into the id map.
  // Called by ClusteringEngine each time it calls incorporateNewCluster().
  void registerCluster(Cluster* cluster);

  // Print the full cluster tree to stdout (calls Cluster::printTree).
  void printTree() const;
};

}  // namespace ipl
```

### `src/ipl/src/physical_hierarchy.cpp`

Implement `findCluster`, `findClusterId`, `registerCluster`, and `printTree`.
All are straightforward map lookups or delegating to `root->printTree()`.

## Files to Modify

### `src/ipl/CMakeLists.txt`

Add the three new `.cpp` files to `ipl_lib`:

```cmake
add_library(ipl_lib
  src/IntegratedPlacer.cpp
  src/metrics.cpp          # NEW
  src/cluster.cpp          # NEW
  src/physical_hierarchy.cpp  # NEW
)
```

### `src/ipl/BUILD`

Add the new sources to the `ipl` `cc_library`:

```python
cc_library(
    name = "ipl",
    srcs = [
        "src/IntegratedPlacer.cpp",
        "src/metrics.cpp",          # NEW
        "src/cluster.cpp",          # NEW
        "src/physical_hierarchy.cpp",  # NEW
    ],
    hdrs = [
        "include/ipl/IntegratedPlacer.h",
        "src/metrics.h",            # NEW
        "src/cluster.h",            # NEW
        "src/physical_hierarchy.h", # NEW
    ],
    includes = [
        "include",
        "src",   # NEW — needed so cluster.h can include metrics.h
    ],
    deps = [
        "//src/odb/src/db",
        "//src/utl",
    ],
)
```

Note: `includes = ["src"]` is added so that `cluster.h` can `#include "metrics.h"`
and `physical_hierarchy.h` can `#include "cluster.h"` without path prefixes.

## Test Requirements

Create `src/ipl/test/cluster_model_test.tcl` — a Tcl-level smoke test that
confirms the new files compile into OpenROAD without error.  Since the data
model has no Tcl commands yet, the test just loads a design and calls
`hello_ipl` to confirm the binary is sound:

```tcl
# cluster_model_test.tcl — confirms S1a compiles cleanly into OpenROAD
source "helpers.tcl"
hello_ipl
```

Add `cluster_model_test` to `src/ipl/test/CMakeLists.txt` and
`src/ipl/test/BUILD`.

Additionally, write a GTest unit test `test/ipl_cluster_test.cc` that
exercises the data model without OpenDB:

### Unit test structure (`test/ipl_cluster_test.cc`)

Cover these cases:

1. **Metrics construction and add()**
   - `Metrics(10, 2, 1000, 5000)` → correct getters
   - `add()` accumulates correctly

2. **Cluster type and naming**
   - Construct a `Cluster(0, "root", nullptr)`
   - `setType(MixedCluster)` → `getTypeString() == "Mixed"`
   - `setType(HardMacroCluster)` → `getTypeString() == "HardMacro"`

3. **Parent/child tree links**
   - Create parent cluster (id=0) and two children (id=1, id=2)
   - `addChild()` → children's parent pointer set to parent
   - `getChildren().size() == 2`
   - `releaseChild()` on child 1 → only child 2 remains, released child has null parent

4. **Metrics propagation**
   - Set metrics on two sibling clusters, manually call `metrics_.add()` on parent
   - Parent's `getNumMacro()` equals sum of children

5. **Connection weights**
   - `addConnection(5, 1.0f)` twice → weight should accumulate to 2.0f
   - `removeConnection(5)` → map is empty

6. **PhysicalHierarchy tree ownership**
   - Construct `PhysicalHierarchy`, set `root = make_unique<Cluster>(0, "root", nullptr)`
   - `registerCluster(root.get())` → `findCluster(0) == root.get()`
   - `findClusterId(nullptr) == -1` (not found)

Add the unit test target to `src/ipl/CMakeLists.txt`:

```cmake
if(ENABLE_TESTS)
  add_executable(ipl_cluster_test
    test/ipl_cluster_test.cc
  )
  target_link_libraries(ipl_cluster_test
    ipl_lib
    GTest::gtest_main
  )
  add_test(NAME ipl_cluster_test COMMAND ipl_cluster_test)
endif()
```

And to `src/ipl/test/BUILD` as a `cc_test`:

```python
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "ipl_cluster_test",
    srcs = ["ipl_cluster_test.cc"],
    deps = [
        "//src/ipl:ipl",
        "@gtest//:gtest_main",
    ],
)
```

## README.md Update

`src/ipl/README.md` already exists from S0. Replace the "Directory layout"
and "Status" sections with the updated content below.  All other sections
(Algorithm overview, Commands, Building and testing) are kept and extended.

Update the status line from:
```
> **Status:** skeleton — only `hello_ipl` is implemented.
```
to:
```
> **Status:** S1a — cluster data model implemented (Metrics, Cluster,
> PhysicalHierarchy). No DB traversal or placement yet.
```

Add a new **"Source file organisation"** section after "Algorithm overview":

```markdown
## Source file organisation

The ipl source tree is split into layers so the clustering engine can be
used independently of the placement engines.

| File(s) | Layer | Purpose |
|---------|-------|---------|
| `src/metrics.h/.cpp` | Data model | Instance-count and area aggregation per cluster |
| `src/cluster.h/.cpp` | Data model | Cluster tree node: type, metrics, leaf instances, parent/child links, connection weights |
| `src/physical_hierarchy.h/.cpp` | Data model | Owns the cluster tree root; holds threshold parameters and id/inst lookup maps |
| `src/cluster_engine.h/.cpp` | Cluster engine | Traverses dbModule hierarchy, builds PhysicalHierarchy (S1b/S1c) |
| `src/IntegratedPlacer.h/.cpp` | Orchestrator | Top-level: sequences clustering → macro placement (S2) → analytical placement (S3) |
| `src/MakeIntegratedPlacer.cpp` | Bootstrap | Registers Tcl/SWIG commands at OpenROAD startup |
```

Also update the "Directory layout" tree to include the new files:

```
src/ipl/
├── CMakeLists.txt
├── BUILD
├── README.md
├── FLOW.md                          ← algorithmic / data-flow diagrams
├── docs/briefs/                     ← implementation briefs
├── include/ipl/
│   ├── IntegratedPlacer.h
│   └── MakeIntegratedPlacer.h
├── src/
│   ├── metrics.h / metrics.cpp      ← S1a: Metrics data class
│   ├── cluster.h / cluster.cpp      ← S1a: Cluster tree node
│   ├── physical_hierarchy.h / .cpp  ← S1a: tree owner + thresholds
│   ├── IntegratedPlacer.cpp         ← S0: top-level orchestrator
│   └── MakeIntegratedPlacer.cpp     ← S0: Tcl bootstrap
└── test/
    ├── CMakeLists.txt
    ├── BUILD
    ├── hello_ipl.tcl / .ok          ← S0: smoke test
    ├── cluster_model_test.tcl / .ok ← S1a: compile smoke test
    └── ipl_cluster_test.cc          ← S1a: GTest unit tests
```

Add one line at the end of README.md:
```markdown
See FLOW.md for data-flow and algorithmic diagrams.
```

## FLOW.md

Create `src/ipl/FLOW.md` at the root of the ipl module (next to README.md).
Follow the eda-lab FLOW.md convention: short intro paragraph, per-file
sections with prose + Mermaid diagram, then a module-level diagram.

```markdown
# Flow: ipl — Integrated Placer

`src/ipl/` is a combined macro and standard-cell global placer.  The
implementation is structured in three layers that can be composed
independently: a **cluster data model** (S1a), a **clustering engine**
that populates it from OpenDB (S1b/S1c), and **placement engines** that
assign coordinates to macros (S2) and standard cells (S3).

This FLOW.md is updated each time a new brief is committed; diagrams
reflect the code as it exists now (S1a — data model only).

## `metrics.h` / `metrics.cpp` — instance count and area accumulation

`Metrics` is a plain data container.  `ClusteringEngine` computes a
`Metrics` for each `dbModule` bottom-up, then attaches it to the
corresponding `Cluster`.  `Cluster::setMetrics()` stores it; there is
no further computation inside `Metrics` itself.

​```mermaid
graph TD
    A["Metrics(num_std_cell, num_macro,<br/>std_cell_area, macro_area)"]
    A --> B["getNumStdCell() / getNumMacro()"]
    A --> C["getStdCellArea() / getMacroArea() / getTotalArea()"]
    A --> D["add(other) — accumulates counts and areas<br/>(used when merging clusters)"]
​```

## `cluster.h` / `cluster.cpp` — physical hierarchy tree node

`Cluster` is the node type for the physical hierarchy tree.  Each node
holds a type classification, aggregated `Metrics`, leaf instance lists,
and tree links.  Physical coordinates are zero-initialised until S2/S3
populate them.

​```mermaid
graph TD
    A["Cluster(id, name, logger)"]
    A --> B["setType(ClusterType)<br/>HardMacroCluster | StdCellCluster<br/>MixedCluster | IOCluster"]
    A --> C["setMetrics(Metrics)"]
    A --> D["addLeafStdCell(dbInst*)<br/>addLeafMacro(dbInst*)"]
    A --> E["addChild(unique_ptr&lt;Cluster&gt;)<br/>→ child.parent = this"]
    A --> F["addConnection(cluster_id, weight)<br/>→ accumulates in ConnectionsMap"]
    E --> G["getChildren() / releaseChild()<br/>releaseChildren()"]
    A --> H["setLocation(x,y)<br/>setDimensions(w,h)<br/>(populated in S2/S3)"]
​```

## `physical_hierarchy.h` / `physical_hierarchy.cpp` — tree owner

`PhysicalHierarchy` owns the root `Cluster` (and therefore the entire
tree via unique_ptr ownership chain).  It also holds the threshold
parameters that `ClusteringEngine` reads to decide when to merge or split,
and the lookup maps that placement engines use for O(1) inst→cluster queries.

​```mermaid
graph TD
    A["PhysicalHierarchy"]
    A --> B["root: unique_ptr&lt;Cluster&gt;<br/>(owns entire tree)"]
    A --> C["Threshold params<br/>base_max/min_macro<br/>base_max/min_std_cell<br/>max_num_level, cluster_size_ratio"]
    A --> D["PhysicalHierarchyMaps<br/>id_to_cluster: unordered_map&lt;int, Cluster*&gt;<br/>inst_to_cluster_id: unordered_map&lt;dbInst*, int&gt;<br/>bterm_to_cluster_id: unordered_map&lt;dbBTerm*, int&gt;"]
    A --> E["Design geometry + flags<br/>die_area, floorplan_shape<br/>has_std_cells, has_unfixed_macros, …"]
    A --> F["registerCluster(Cluster*)<br/>→ inserts into id_to_cluster map"]
    A --> G["findCluster(id) → Cluster*<br/>findClusterId(dbInst*) → int"]
​```

## Module-level: layer interactions (S1a state)

​```mermaid
sequenceDiagram
    participant IP as IntegratedPlacer
    participant PH as PhysicalHierarchy
    participant C  as Cluster tree

    Note over IP,C: S1a — data model only; no DB traversal yet
    IP->>PH: set threshold params<br/>(base_max_macro, max_num_level, …)
    Note over IP,PH: S1b: ClusteringEngine reads thresholds,<br/>traverses dbModule tree, fills Cluster tree
    IP->>PH: registerCluster(cluster*)
    PH->>C: root owns tree via unique_ptr chain
    Note over IP,C: S2: MacroPlacer reads PH, calls<br/>cluster->setLocation() on macro clusters
    Note over IP,C: S3: AnalyticalPlacer reads PH,<br/>uses inst_to_cluster_id for density kernels
​```
```

## Build Verification

```bash
# Bazel
bazel build //:openroad
bazel test //src/ipl/test:ipl_cluster_test --test_output=all
bazel test //src/ipl/test:cluster_model_test --test_output=all

# CMake
cmake --build build -j$(nproc)
ctest --test-dir build -R "ipl_cluster_test|cluster_model_test" --output-on-failure
```

## Commit

```bash
git add src/ipl/src/metrics.h \
        src/ipl/src/metrics.cpp \
        src/ipl/src/cluster.h \
        src/ipl/src/cluster.cpp \
        src/ipl/src/physical_hierarchy.h \
        src/ipl/src/physical_hierarchy.cpp \
        src/ipl/test/cluster_model_test.tcl \
        src/ipl/test/cluster_model_test.ok \
        src/ipl/test/ipl_cluster_test.cc \
        src/ipl/CMakeLists.txt \
        src/ipl/BUILD \
        src/ipl/test/CMakeLists.txt \
        src/ipl/test/BUILD \
        src/ipl/README.md \
        src/ipl/FLOW.md
git commit -s -m "ipl: S1a cluster data model — Metrics, Cluster, PhysicalHierarchy

Pure C++ data model for the ipl physical hierarchy tree.
No DB traversal, no Tcl commands, no placement logic.

- Metrics: std-cell/macro instance counts and areas (dbu²)
- Cluster: tree node with type, Metrics, leaf dbInst* lists,
  parent/child ownership, physical coords (zero until S2/S3),
  and inter-cluster connection weight map
- PhysicalHierarchy: owns the cluster tree, holds threshold
  parameters for ClusteringEngine, provides id/inst lookup maps
- GTest unit test covering construction, tree links, metrics
  accumulation, connection weights, hierarchy ownership

Architectural intent: Metrics + Cluster + PhysicalHierarchy form a
standalone reusable layer.  ClusteringEngine (S1b) fills the tree from
OpenDB; MacroPlacer (S2) and AnalyticalPlacer (S3) consume it.
Neither placement stage is a dependency of this layer."
```

## Deliverables Checklist

- [ ] `src/ipl/src/metrics.h` and `metrics.cpp` created and correct
- [ ] `src/ipl/src/cluster.h` and `cluster.cpp` created and correct
- [ ] `src/ipl/src/physical_hierarchy.h` and `.cpp` created and correct
- [ ] `ipl_lib` in `CMakeLists.txt` and `BUILD` updated with all three new `.cpp`/`.h`
- [ ] `includes = ["src"]` added to the `ipl` Bazel target
- [ ] `test/ipl_cluster_test.cc` created with all 6 unit test cases passing
- [ ] `test/cluster_model_test.tcl` + `.ok` created and passing
- [ ] `bazel build //:openroad` clean (no errors, no new warnings)
- [ ] `bazel test //src/ipl/test:ipl_cluster_test` PASSED
- [ ] `bazel test //src/ipl/test:cluster_model_test` PASSED
- [ ] CMake build and ctest also clean
- [ ] `git commit -s` on branch `ipl` with message above
- [ ] `src/ipl/README.md` updated: status line, source file organisation table, directory layout tree, "See FLOW.md" pointer
- [ ] `src/ipl/FLOW.md` created with per-file prose + Mermaid diagrams for `Metrics`, `Cluster`, `PhysicalHierarchy`, and the module-level sequence diagram (exact content specified in the "FLOW.md" section above)

## Hard Gate

Do not proceed to S1b until:
1. All unit test cases in `ipl_cluster_test` are green under both Bazel and ctest
2. `bazel build //:openroad` is clean with no new warnings
3. `src/ipl/README.md` contains the source-file organisation table and updated directory layout
4. `src/ipl/FLOW.md` exists with valid Mermaid syntax for all four diagrams
5. The S1a commit is on branch `ipl` and `git status` is clean
