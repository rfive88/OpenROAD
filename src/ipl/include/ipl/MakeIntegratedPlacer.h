// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Application startup infrastructure for the Integrated Placer (ipl).
// OpenRoad::init() uses initIntegratedPlacer() to register the ipl Tcl
// commands and evaluate the ipl.tcl script.  Any tools that ipl depends on
// at initialisation time would be added as extra arguments here and passed
// from OpenRoad::init() after those tools have been initialised.

#pragma once

#include "tcl.h"

namespace ipl {

class IntegratedPlacer;

void initIntegratedPlacer(Tcl_Interp* tcl_interp);

}  // namespace ipl
