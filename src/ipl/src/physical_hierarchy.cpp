// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// PhysicalHierarchy implementation: lookup maps and tree printing.

#include "physical_hierarchy.h"

#include "odb/db.h"

namespace ipl {

Cluster* PhysicalHierarchy::findCluster(int id) const
{
  auto it = maps.id_to_cluster.find(id);
  return it == maps.id_to_cluster.end() ? nullptr : it->second;
}

int PhysicalHierarchy::findClusterId(odb::dbInst* inst) const
{
  auto it = maps.inst_to_cluster_id.find(inst);
  return it == maps.inst_to_cluster_id.end() ? -1 : it->second;
}

void PhysicalHierarchy::registerCluster(Cluster* cluster)
{
  maps.id_to_cluster[cluster->getId()] = cluster;
}

void PhysicalHierarchy::printTree() const
{
  if (root != nullptr) {
    root->printTree();
  }
}

}  // namespace ipl
