# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

# hello_ipl.tcl — smoke test for the ipl skeleton.
#
# Verifies that the ipl module is linked into OpenROAD and that the
# hello_ipl Tcl command is callable.  The command logs "hello_ipl" via
# utl::Logger::report(), which this test captures and checks.

# No LEF/DEF is required — hello_ipl only needs the logger.
hello_ipl
