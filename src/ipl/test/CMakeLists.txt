# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

or_integration_tests(
  "ipl"
  TESTS
    hello_ipl
    cluster_model_test
)

# Unit tests for the cluster data model (S1a).  These link ipl_lib directly
# so they run without the Tcl/GUI stack.
add_executable(ipl_cluster_test ipl_cluster_test.cc)

target_link_libraries(ipl_cluster_test
    GTest::gtest
    GTest::gtest_main
    ipl_lib
    utl_lib
)

gtest_discover_tests(ipl_cluster_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test ipl_cluster_test)
