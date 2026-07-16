// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// SWIG interface file for ipl (Integrated Placer).
//
// SWIG uses this file to generate the C-level Tcl bindings that are
// registered by Ipl_Init().  Keep argument types simple (int, float,
// const char*) to avoid complex SWIG typemap handling.  High-level
// argument parsing and validation lives in ipl.tcl instead.

%{
#include "ord/OpenRoad.hh"
#include "ipl/IntegratedPlacer.h"

// Forward declaration of the accessor defined in OpenRoad.i.
namespace ord {
ipl::IntegratedPlacer* getIntegratedPlacer();
}
%}

%include "../../Exception.i"

%inline %{

namespace ipl {

// hello_ipl_cmd -------------------------------------------------------------
// Called by the "hello_ipl" Tcl command (defined in ipl.tcl).
// Retrieves the IntegratedPlacer singleton from OpenRoad and calls helloIpl().
void hello_ipl_cmd()
{
  ipl::IntegratedPlacer* ipl = ord::getIntegratedPlacer();
  ipl->helloIpl();
}

}  // namespace ipl

%}  // inline
