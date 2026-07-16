# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

# cluster_model_test.tcl — confirms the S1a cluster data model compiles and
# links cleanly into the OpenROAD binary.
#
# The data model (Metrics, Cluster, PhysicalHierarchy) has no Tcl commands
# yet, so this test only checks that the binary is sound by calling the
# hello_ipl diagnostic.  It will grow real assertions once ClusteringEngine
# (S1b) exposes a command.

hello_ipl
