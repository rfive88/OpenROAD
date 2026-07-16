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

```mermaid
graph TD
    A["Metrics(num_std_cell, num_macro,<br/>std_cell_area, macro_area)"]
    A --> B["getNumStdCell() / getNumMacro()"]
    A --> C["getStdCellArea() / getMacroArea() / getTotalArea()"]
    A --> D["add(other) — accumulates counts and areas<br/>(used when merging clusters)"]
```

## `cluster.h` / `cluster.cpp` — physical hierarchy tree node

`Cluster` is the node type for the physical hierarchy tree.  Each node
holds a type classification, aggregated `Metrics`, leaf instance lists,
and tree links.  Physical coordinates are zero-initialised until S2/S3
populate them.

```mermaid
graph TD
    A["Cluster(id, name, logger)"]
    A --> B["setType(ClusterType)<br/>HardMacroCluster | StdCellCluster<br/>MixedCluster | IOCluster"]
    A --> C["setMetrics(Metrics)"]
    A --> D["addLeafStdCell(dbInst*)<br/>addLeafMacro(dbInst*)"]
    A --> E["addChild(unique_ptr&lt;Cluster&gt;)<br/>→ child.parent = this"]
    A --> F["addConnection(cluster_id, weight)<br/>→ accumulates in ConnectionsMap"]
    E --> G["getChildren() / releaseChild()<br/>releaseChildren()"]
    A --> H["setLocation(x,y)<br/>setDimensions(w,h)<br/>(populated in S2/S3)"]
```

## `physical_hierarchy.h` / `physical_hierarchy.cpp` — tree owner

`PhysicalHierarchy` owns the root `Cluster` (and therefore the entire
tree via unique_ptr ownership chain).  It also holds the threshold
parameters that `ClusteringEngine` reads to decide when to merge or split,
and the lookup maps that placement engines use for O(1) inst→cluster queries.

```mermaid
graph TD
    A["PhysicalHierarchy"]
    A --> B["root: unique_ptr&lt;Cluster&gt;<br/>(owns entire tree)"]
    A --> C["Threshold params<br/>base_max/min_macro<br/>base_max/min_std_cell<br/>max_num_level, cluster_size_ratio"]
    A --> D["PhysicalHierarchyMaps<br/>id_to_cluster: unordered_map&lt;int, Cluster*&gt;<br/>inst_to_cluster_id: unordered_map&lt;dbInst*, int&gt;<br/>bterm_to_cluster_id: unordered_map&lt;dbBTerm*, int&gt;"]
    A --> E["Design geometry + flags<br/>die_area, floorplan_shape<br/>has_std_cells, has_unfixed_macros, …"]
    A --> F["registerCluster(Cluster*)<br/>→ inserts into id_to_cluster map"]
    A --> G["findCluster(id) → Cluster*<br/>findClusterId(dbInst*) → int"]
```

## Module-level: layer interactions (S1a state)

```mermaid
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
```
