# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

# ipl.tcl — Tcl command definitions for the Integrated Placer (ipl).
#
# Each proc defined here wraps the corresponding SWIG-generated C command.
# Argument parsing, validation, and help text live here; the C layer only
# receives simple scalar values.

sta::define_cmd_args "hello_ipl" {}

proc hello_ipl { args } {
  sta::parse_key_args "hello_ipl" args \
    keys {} flags {}
  ipl::hello_ipl_cmd
}
