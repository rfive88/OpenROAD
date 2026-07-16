// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// IntegratedPlacer implementation.
//
// This file contains the core placer logic.  At the skeleton stage only
// helloIpl() is implemented.  Placement algorithm passes (macro clustering,
// simulated annealing, analytical cell placement, legalisation) will be
// added here — or in separate functional .cpp files — as the implementation
// briefs in src/ipl/doc/ are executed.

#include "ipl/IntegratedPlacer.h"

#include "utl/Logger.h"

namespace ipl {

// Tool logger message id range reserved for ipl: 1–199.
// Sub-ranges will be partitioned per brief as the tool grows.
constexpr int kHelloIplMsgId = 1;

IntegratedPlacer::IntegratedPlacer(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
}

IntegratedPlacer::~IntegratedPlacer() = default;

// helloIpl ------------------------------------------------------------------
// Diagnostic entry point wired to the Tcl command "hello_ipl".
// Logs a report-level message so the output is visible at the default
// verbosity, confirming that ipl is correctly linked and Tcl-accessible.
void IntegratedPlacer::helloIpl()
{
  logger_->report("hello_ipl");
}

}  // namespace ipl
