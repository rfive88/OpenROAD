# Integrated Placer (ipl)

`ipl` is a combined macro and standard-cell global placer for OpenROAD.
It replaces the separate `mpl` (Hier-RTLMP macro placer) and `gpl`
(RePlAce global placer) passes with a single co-optimised flow in which
macro positions and standard-cell positions are solved together.

> **Status:** skeleton — only `hello_ipl` is implemented.
> Functional placement passes are being developed in staged briefs
> under `src/ipl/doc/`.

## Algorithm overview

| Stage | Algorithm | Reference |
|-------|-----------|-----------|
| Hierarchy extraction | RTL-MP / Hier-RTLMP multilevel clustering | ISPD 2022, 2023 |
| Macro placement | Simulated annealing over hierarchical soft/hard macros | Hier-RTLMP 2023 |
| Mixed-size global placement | ePlace-MS electrostatic density + Nesterov / BB optimiser | TCAD 2015, ICCAD 2023 |
| Routability refinement | RePlAce constraint-oriented local smoothing | TCAD 2019 |

Paper PDFs are in `~/projects/eda-lab/docs/papers/`.

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
├── README.md                        # this file
├── doc/                             # implementation briefs (one per stage)
├── include/ipl/
│   ├── IntegratedPlacer.h           # public API class
│   └── MakeIntegratedPlacer.h       # Tcl bootstrap entry point
├── src/
│   ├── IntegratedPlacer.cpp         # core placer implementation
│   ├── MakeIntegratedPlacer.cpp     # Tcl/SWIG registration
│   ├── ipl.i                        # SWIG interface → Tcl commands
│   └── ipl.tcl                      # Tcl command wrappers
└── test/
    ├── CMakeLists.txt
    ├── hello_ipl.tcl                # smoke test
    └── hello_ipl.ok                 # expected output
```

## Building and testing

```bash
# Configure (from the OpenROAD repo root)
cmake -B build -DENABLE_TESTS=ON
cmake --build build -j$(nproc)

# Run the ipl smoke test
ctest --test-dir build -R hello_ipl --output-on-failure
```
