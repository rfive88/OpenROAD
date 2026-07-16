# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "hello_ipl",
    "cluster_model_test",
]

filegroup(
    name = "test_resources",
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]

# Unit tests for the cluster data model (S1a).  Links //src/ipl directly so
# it runs without the Tcl/GUI stack.
cc_test(
    name = "ipl_cluster_test",
    srcs = ["ipl_cluster_test.cc"],
    deps = [
        "//src/ipl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
