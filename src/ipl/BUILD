# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# ipl: core placer logic — no GUI dependencies.
# C++ unit tests link against this target directly to avoid pulling in
# the full OpenROAD GUI stack.
cc_library(
    name = "ipl",
    srcs = [
        "src/IntegratedPlacer.cpp",
        "src/cluster.cpp",
        "src/metrics.cpp",
        "src/physical_hierarchy.cpp",
    ],
    hdrs = [
        "include/ipl/IntegratedPlacer.h",
        "src/cluster.h",
        "src/metrics.h",
        "src/physical_hierarchy.h",
    ],
    includes = [
        "include",
        # so cluster.h can include "metrics.h" without a path prefix
        "src",
    ],
    deps = [
        "//src/odb/src/db",
        "//src/utl",
    ],
)

# ui: Tcl/SWIG registration layer.  OpenROAD's top-level BUILD.bazel
# depends on this target (as //src/ipl:ui).
cc_library(
    name = "ui",
    srcs = [
        "src/MakeIntegratedPlacer.cpp",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/ipl/MakeIntegratedPlacer.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":ipl",
        "//:ord",
        "//src/utl",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/ipl.tcl",
    ],
    char_array_name = "ipl_tcl_inits",
    namespace = "ipl",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/ipl.i",
        "//:error_swig",
    ],
    module = "ipl",
    namespace_prefix = "ipl",
    root_swig_src = "src/ipl.i",
    swig_includes = [
        "src/ipl/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)
