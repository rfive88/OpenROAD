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
  std::unique_ptr<Cluster> root;  // owns the entire tree
  PhysicalHierarchyMaps maps;

  // -----------------------------------------------------------------------
  // Clustering threshold parameters
  // These are set by the caller (IntegratedPlacer) before handing the
  // struct to ClusteringEngine.  ClusteringEngine will compute per-level
  // derived limits (max_macro_, min_macro_, etc.) from these base values.
  // See Hier-RTLMP §III-C for the derivation.
  // -----------------------------------------------------------------------
  int base_max_macro{0};           // max hard macros per cluster at level 0
  int base_min_macro{0};           // min hard macros per cluster at level 0
  int base_max_std_cell{0};        // max std cells per cluster at level 0
  int base_min_std_cell{0};        // min std cells per cluster at level 0
  int max_num_level{0};            // max hierarchy depth to generate
  int large_net_threshold{0};      // nets with more pins than this are ignored
  float cluster_size_ratio{0.0f};  // per-level size scaling ratio
  float cluster_size_tolerance{0.0f};  // merge tolerance fraction

  // -----------------------------------------------------------------------
  // Design-level data (populated by ClusteringEngine in S1b)
  // -----------------------------------------------------------------------
  odb::Rect die_area;               // from dbBlock::getDieArea()
  odb::Rect floorplan_shape;        // usable core area (may differ from die)
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
