// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Unit tests for the ipl cluster data model (S1a).
//
// The data model is deliberately free of OpenDB traversal and placement
// logic, so these tests exercise it standalone: no dbDatabase, no logger.

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "cluster.h"
#include "gtest/gtest.h"
#include "metrics.h"
#include "physical_hierarchy.h"
#include "spdlog/sinks/ostream_sink.h"
#include "utl/Logger.h"

namespace ipl {
namespace {

TEST(MetricsTest, ConstructionAndAdd)
{
  Metrics metrics(10, 2, 1000, 5000);
  EXPECT_EQ(metrics.getNumStdCell(), 10u);
  EXPECT_EQ(metrics.getNumMacro(), 2u);
  EXPECT_EQ(metrics.getStdCellArea(), 1000);
  EXPECT_EQ(metrics.getMacroArea(), 5000);
  EXPECT_EQ(metrics.getTotalArea(), 6000);
  EXPECT_FALSE(metrics.empty());

  Metrics empty;
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(empty.getTotalArea(), 0);

  metrics.add(Metrics(5, 1, 500, 2000));
  EXPECT_EQ(metrics.getNumStdCell(), 15u);
  EXPECT_EQ(metrics.getNumMacro(), 3u);
  EXPECT_EQ(metrics.getStdCellArea(), 1500);
  EXPECT_EQ(metrics.getMacroArea(), 7000);
  EXPECT_EQ(metrics.getTotalArea(), 8500);
}

TEST(ClusterTest, TypeAndNaming)
{
  Cluster cluster(0, "root", nullptr);
  EXPECT_EQ(cluster.getId(), 0);
  EXPECT_EQ(cluster.getName(), "root");

  cluster.setName("renamed");
  EXPECT_EQ(cluster.getName(), "renamed");

  cluster.setType(MixedCluster);
  EXPECT_EQ(cluster.getType(), MixedCluster);
  EXPECT_EQ(cluster.getTypeString(), "Mixed");

  cluster.setType(HardMacroCluster);
  EXPECT_EQ(cluster.getTypeString(), "HardMacro");

  cluster.setType(StdCellCluster);
  EXPECT_EQ(cluster.getTypeString(), "StdCell");

  cluster.setType(IOCluster);
  EXPECT_EQ(cluster.getTypeString(), "IO");
}

TEST(ClusterTest, ParentChildLinks)
{
  Cluster parent(0, "parent", nullptr);
  auto child1 = std::make_unique<Cluster>(1, "child1", nullptr);
  auto child2 = std::make_unique<Cluster>(2, "child2", nullptr);
  Cluster* child1_ptr = child1.get();
  Cluster* child2_ptr = child2.get();

  EXPECT_TRUE(parent.isLeaf());

  parent.addChild(std::move(child1));
  parent.addChild(std::move(child2));

  EXPECT_FALSE(parent.isLeaf());
  EXPECT_EQ(parent.getChildren().size(), 2u);
  EXPECT_EQ(child1_ptr->getParent(), &parent);
  EXPECT_EQ(child2_ptr->getParent(), &parent);
  EXPECT_EQ(parent.getParent(), nullptr);

  std::unique_ptr<Cluster> released = parent.releaseChild(child1_ptr);
  ASSERT_NE(released, nullptr);
  EXPECT_EQ(released.get(), child1_ptr);
  EXPECT_EQ(released->getParent(), nullptr);
  ASSERT_EQ(parent.getChildren().size(), 1u);
  EXPECT_EQ(parent.getChildren()[0].get(), child2_ptr);

  // Releasing a cluster that is not a child yields nullptr.
  EXPECT_EQ(parent.releaseChild(child1_ptr), nullptr);

  UniqueClusterVector all = parent.releaseChildren();
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0]->getParent(), nullptr);
  EXPECT_TRUE(parent.isLeaf());
}

TEST(ClusterTest, MetricsPropagation)
{
  Cluster parent(0, "parent", nullptr);
  Cluster child1(1, "child1", nullptr);
  Cluster child2(2, "child2", nullptr);

  child1.setMetrics(Metrics(100, 2, 10000, 40000));
  child2.setMetrics(Metrics(50, 3, 5000, 60000));

  Metrics rolled_up = child1.getMetrics();
  rolled_up.add(child2.getMetrics());
  parent.setMetrics(rolled_up);

  EXPECT_EQ(parent.getNumStdCell(),
            child1.getNumStdCell() + child2.getNumStdCell());
  EXPECT_EQ(parent.getNumMacro(), child1.getNumMacro() + child2.getNumMacro());
  EXPECT_EQ(parent.getTotalArea(),
            child1.getTotalArea() + child2.getTotalArea());
}

TEST(ClusterTest, ConnectionWeights)
{
  Cluster cluster(0, "cluster", nullptr);
  EXPECT_TRUE(cluster.getConnectionsMap().empty());

  cluster.addConnection(5, 1.0f);
  cluster.addConnection(5, 1.0f);
  ASSERT_EQ(cluster.getConnectionsMap().size(), 1u);
  EXPECT_FLOAT_EQ(cluster.getConnectionsMap().at(5), 2.0f);

  cluster.addConnection(6, 3.5f);
  EXPECT_EQ(cluster.getConnectionsMap().size(), 2u);

  cluster.removeConnection(5);
  EXPECT_EQ(cluster.getConnectionsMap().count(5), 0u);
  EXPECT_EQ(cluster.getConnectionsMap().size(), 1u);

  cluster.clearConnections();
  EXPECT_TRUE(cluster.getConnectionsMap().empty());
}

TEST(ClusterTest, PhysicalCoordsDefaultToZero)
{
  Cluster cluster(0, "cluster", nullptr);
  EXPECT_EQ(cluster.getX(), 0);
  EXPECT_EQ(cluster.getY(), 0);
  EXPECT_EQ(cluster.getWidth(), 0);
  EXPECT_EQ(cluster.getHeight(), 0);

  cluster.setLocation(100, 200);
  cluster.setDimensions(300, 400);
  EXPECT_EQ(cluster.getX(), 100);
  EXPECT_EQ(cluster.getY(), 200);
  EXPECT_EQ(cluster.getWidth(), 300);
  EXPECT_EQ(cluster.getHeight(), 400);
}

// Builds root -> child -> grandchild, one cluster per type, so printTree has
// both a type spread and two levels of nesting to render.
std::unique_ptr<Cluster> makeThreeLevelTree(utl::Logger* logger)
{
  auto root = std::make_unique<Cluster>(0, "root", logger);
  root->setType(MixedCluster);
  root->setMetrics(Metrics(150, 5, 15000, 100000));

  auto child = std::make_unique<Cluster>(1, "child", logger);
  child->setType(HardMacroCluster);
  child->setMetrics(Metrics(0, 5, 0, 100000));

  auto grandchild = std::make_unique<Cluster>(2, "grandchild", logger);
  grandchild->setType(StdCellCluster);
  grandchild->setMetrics(Metrics(150, 0, 15000, 0));

  child->addChild(std::move(grandchild));
  root->addChild(std::move(child));
  return root;
}

constexpr const char* kExpectedTree
    = "[Mixed] root  (macros=5 cells=150)\n"
      "  [HardMacro] child  (macros=5 cells=0)\n"
      "    [StdCell] grandchild  (macros=0 cells=150)\n";

// The production path: with a logger attached, printTree reports through it.
// An spdlog ostream sink collects the output, the same mechanism web_serve.cpp
// uses to tap the logger.  utl::Logger formats with pattern "%v", so what the
// sink sees is exactly the reported line.
TEST(ClusterTest, PrintTreeReportsThroughLoggerWhenAttached)
{
  std::ostringstream captured;
  utl::Logger logger;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(captured);
  logger.addSink(sink);

  std::unique_ptr<Cluster> root = makeThreeLevelTree(&logger);
  root->printTree();
  logger.removeSink(sink);

  EXPECT_EQ(captured.str(), kExpectedTree);
}

// The fallback path: with no logger, printTree still emits on std::cout, which
// is what keeps it observable from logger-free unit tests.  Both paths must
// render identically.
TEST(ClusterTest, PrintTreeFallsBackToStdoutWithoutLogger)
{
  std::unique_ptr<Cluster> root = makeThreeLevelTree(nullptr);

  testing::internal::CaptureStdout();
  root->printTree();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, kExpectedTree);
}

TEST(PhysicalHierarchyTest, PrintTreeToleratesNullRoot)
{
  PhysicalHierarchy hierarchy;
  ASSERT_EQ(hierarchy.root, nullptr);

  testing::internal::CaptureStdout();
  hierarchy.printTree();
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

TEST(PhysicalHierarchyTest, TreeOwnershipAndLookup)
{
  PhysicalHierarchy hierarchy;
  EXPECT_EQ(hierarchy.root, nullptr);
  EXPECT_EQ(hierarchy.findCluster(0), nullptr);

  hierarchy.root = std::make_unique<Cluster>(0, "root", nullptr);
  hierarchy.registerCluster(hierarchy.root.get());
  EXPECT_EQ(hierarchy.findCluster(0), hierarchy.root.get());
  EXPECT_EQ(hierarchy.findCluster(1), nullptr);

  auto child = std::make_unique<Cluster>(1, "child", nullptr);
  Cluster* child_ptr = child.get();
  hierarchy.root->addChild(std::move(child));
  hierarchy.registerCluster(child_ptr);
  EXPECT_EQ(hierarchy.findCluster(1), child_ptr);

  // No instances are assigned, so any lookup misses.
  EXPECT_EQ(hierarchy.findClusterId(nullptr), -1);
}

}  // namespace
}  // namespace ipl
