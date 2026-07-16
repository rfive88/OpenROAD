// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Cluster implementation: tree links, membership lists and connection
// weights.  No DB traversal and no placement logic live here — the tree is
// filled in by ClusteringEngine (S1b/S1c) and consumed by the macro placer
// (S2) and the analytical placer (S3).

#include "cluster.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"

namespace ipl {

Cluster::Cluster(int id, std::string name, utl::Logger* logger)
    : id_(id), name_(std::move(name)), logger_(logger)
{
}

std::string Cluster::getTypeString() const
{
  switch (type_) {
    case HardMacroCluster:
      return "HardMacro";
    case StdCellCluster:
      return "StdCell";
    case MixedCluster:
      return "Mixed";
    case IOCluster:
      return "IO";
  }
  return "Unknown";
}

// Logical modules ------------------------------------------------------------

void Cluster::addDbModule(odb::dbModule* module)
{
  db_modules_.push_back(module);
}

const std::vector<odb::dbModule*>& Cluster::getDbModules() const
{
  return db_modules_;
}

void Cluster::clearDbModules()
{
  db_modules_.clear();
}

// Leaf instances -------------------------------------------------------------

void Cluster::addLeafStdCell(odb::dbInst* inst)
{
  leaf_std_cells_.push_back(inst);
}

void Cluster::addLeafMacro(odb::dbInst* inst)
{
  leaf_macros_.push_back(inst);
}

const std::vector<odb::dbInst*>& Cluster::getLeafStdCells() const
{
  return leaf_std_cells_;
}

const std::vector<odb::dbInst*>& Cluster::getLeafMacros() const
{
  return leaf_macros_;
}

void Cluster::clearLeafInstances()
{
  leaf_std_cells_.clear();
  leaf_macros_.clear();
}

// Tree links -----------------------------------------------------------------

void Cluster::addChild(std::unique_ptr<Cluster> child)
{
  child->setParent(this);
  children_.push_back(std::move(child));
}

// Detaches target from this cluster and transfers ownership to the caller.
// Returns nullptr if target is not a child of this cluster.
std::unique_ptr<Cluster> Cluster::releaseChild(const Cluster* target)
{
  auto it = std::find_if(children_.begin(),
                         children_.end(),
                         [target](const std::unique_ptr<Cluster>& child) {
                           return child.get() == target;
                         });

  if (it == children_.end()) {
    return nullptr;
  }

  std::unique_ptr<Cluster> released = std::move(*it);
  children_.erase(it);
  released->setParent(nullptr);
  return released;
}

// Detaches every child at once; this cluster becomes a leaf.
UniqueClusterVector Cluster::releaseChildren()
{
  UniqueClusterVector released = std::move(children_);
  children_.clear();
  for (auto& child : released) {
    child->setParent(nullptr);
  }
  return released;
}

// Connections ----------------------------------------------------------------

// Weights accumulate: a cluster pair may be connected by many nets, and each
// crossing net adds to the same entry.
void Cluster::addConnection(int other_cluster_id, float weight)
{
  connections_[other_cluster_id] += weight;
}

void Cluster::removeConnection(int other_cluster_id)
{
  connections_.erase(other_cluster_id);
}

// Debug ----------------------------------------------------------------------

// Prints this cluster and its subtree, two spaces per level.  Falls back to
// std::cout when no logger is attached (the data model is constructible
// without one, e.g. in unit tests).
void Cluster::printTree(int indent) const
{
  const std::string line = std::string(indent * 2, ' ') + "[" + getTypeString()
                           + "] " + name_
                           + "  (macros=" + std::to_string(getNumMacro())
                           + " cells=" + std::to_string(getNumStdCell()) + ")";

  if (logger_ != nullptr) {
    logger_->report("{}", line);
  } else {
    std::cout << line << std::endl;
  }

  for (const auto& child : children_) {
    child->printTree(indent + 1);
  }
}

}  // namespace ipl
