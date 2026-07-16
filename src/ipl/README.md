# Integrated Placer (ipl)

`ipl` is a combined macro and standard-cell global placer for OpenROAD.
It replaces the separate `mpl` (Hier-RTLMP macro placer) and `gpl`
(RePlAce global placer) passes with a single co-optimised flow in which
macro positions and standard-cell positions are solved together.

> **Status:** S1a — cluster data model implemented (Metrics, Cluster,
> PhysicalHierarchy). No DB traversal or placement yet.
> Functional placement passes are being developed in staged briefs
> under `src/ipl/docs/briefs/`.

## Algorithm overview

| Stage | Algorithm | Reference |
|-------|-----------|-----------|
| Hierarchy extraction | RTL-MP / Hier-RTLMP multilevel clustering | ISPD 2022, 2023 |
| Macro placement | Simulated annealing over hierarchical soft/hard macros | Hier-RTLMP 2023 |
| Mixed-size global placement | ePlace-MS electrostatic density + Nesterov / BB optimiser | TCAD 2015, ICCAD 2023 |
| Routability refinement | RePlAce constraint-oriented local smoothing | TCAD 2019 |

Paper PDFs are in `~/projects/eda-lab/docs/papers/`.

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

## Commands

### `hello_ipl`

Diagnostic command.  Prints `hello_ipl` to confirm the tool is registered
and Tcl-accessible.  Will be removed once the first placement stage is
implemented.

```tcl
hello_ipl
```

## Directory layout

```
src/ipl/
├── CMakeLists.txt
├── BUILD
├── README.md                        # this file
├── FLOW.md                          # algorithmic / data-flow diagrams
├── docs/briefs/                     # implementation briefs (one per stage)
├── include/ipl/
│   ├── IntegratedPlacer.h           # public API class
│   └── MakeIntegratedPlacer.h       # Tcl bootstrap entry point
├── src/
│   ├── metrics.h / metrics.cpp      # S1a: Metrics data class
│   ├── cluster.h / cluster.cpp      # S1a: Cluster tree node
│   ├── physical_hierarchy.h / .cpp  # S1a: tree owner + thresholds
│   ├── IntegratedPlacer.cpp         # S0: top-level orchestrator
│   ├── MakeIntegratedPlacer.cpp     # S0: Tcl/SWIG registration
│   ├── ipl.i                        # SWIG interface → Tcl commands
│   └── ipl.tcl                      # Tcl command wrappers
└── test/
    ├── CMakeLists.txt
    ├── BUILD
    ├── hello_ipl.tcl / .ok          # S0: smoke test
    ├── cluster_model_test.tcl / .ok # S1a: compile smoke test
    └── ipl_cluster_test.cc          # S1a: GTest unit tests
```

## Building and testing

```bash
# Configure (from the OpenROAD repo root)
cmake -B build -DENABLE_TESTS=ON
cmake --build build -j$(nproc)

# Run the ipl tests
ctest --test-dir build -R "ipl|hello_ipl|cluster_model_test" --output-on-failure
```

Under Bazel:

```bash
bazel build //:openroad
bazel test //src/ipl/test:ipl_cluster_test
bazel test //src/ipl/test:cluster_model_test-tcl_test
```

See FLOW.md for data-flow and algorithmic diagrams.
