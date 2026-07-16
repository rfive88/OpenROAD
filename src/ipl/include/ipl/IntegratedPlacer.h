// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// IntegratedPlacer: combined macro and standard-cell placer (ipl).
//
// This class is the top-level API object for the ipl tool.  It is owned by
// OpenRoad and accessed from Tcl via the SWIG-generated ipl.i interface.
//
// Architecture overview:
//   - Macro placement uses a hierarchical RTL-driven approach derived from
//     Hier-RTLMP (mpl2), exploiting RTL hierarchy and dataflow.
//   - Standard-cell global placement uses an electrostatics-based analytical
//     engine derived from ePlace / RePlAce (gpl), solved via FFT Poisson.
//   - Both stages share a common density/wirelength gradient infrastructure
//     so macro and cell positions are co-optimised.
//
// At this skeleton stage only the "hello_ipl" diagnostic command is wired up.
// Functional placement passes will be added in subsequent implementation
// briefs under src/ipl/doc/.

#pragma once

#include <memory>
#include <string>

#include "odb/db.h"
#include "utl/Logger.h"

namespace ipl {

// IntegratedPlacer holds the top-level placer state and exposes the
// command entry points called from ipl.i / ipl.tcl.
class IntegratedPlacer
{
 public:
  // Constructs the placer.  db and logger are owned by OpenRoad and
  // must outlive this object.
  IntegratedPlacer(odb::dbDatabase* db, utl::Logger* logger);
  ~IntegratedPlacer();

  // Diagnostic command: prints "hello_ipl" to confirm the tool is
  // registered and callable from Tcl.  Will be removed once the first
  // real placement stage is implemented.
  void helloIpl();

 private:
  odb::dbDatabase* db_ = nullptr;  // OpenDB design database (not owned)
  utl::Logger* logger_ = nullptr;  // shared logger (not owned)
};

}  // namespace ipl
