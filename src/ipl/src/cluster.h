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

class Cluster;

// ConnectionsMap: cluster_id → aggregated net-crossing connection weight.
// Populated by ClusteringEngine::buildNetListConnections() in S1b.
using ConnectionsMap = std::map<int, float>;
using UniqueClusterVector = std::vector<std::unique_ptr<Cluster>>;

class Cluster
{
 public:
  Cluster(int id, std::string name, utl::Logger* logger);

  // Identity
  int getId() const { return id_; }
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
  unsigned int getNumMacro() const { return metrics_.getNumMacro(); }
  int64_t getTotalArea() const { return metrics_.getTotalArea(); }

  // Logical module associations (one cluster may span multiple dbModules)
  void addDbModule(odb::dbModule* module);
  const std::vector<odb::dbModule*>& getDbModules() const;
  void clearDbModules();

  // Leaf instance membership
  void addLeafStdCell(odb::dbInst* inst);
  void addLeafMacro(odb::dbInst* inst);
  const std::vector<odb::dbInst*>& getLeafStdCells() const;
  const std::vector<odb::dbInst*>& getLeafMacros() const;
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
  int getWidth() const { return width_; }
  int getHeight() const { return height_; }
  void setLocation(int x, int y)
  {
    x_ = x;
    y_ = y;
  }
  void setDimensions(int width, int height)
  {
    width_ = width;
    height_ = height;
  }

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

  Cluster* parent_{nullptr};      // raw, non-owning
  UniqueClusterVector children_;  // owned

  ConnectionsMap connections_;  // net-crossing weights

  utl::Logger* logger_{nullptr};  // non-owning
};

}  // namespace ipl
