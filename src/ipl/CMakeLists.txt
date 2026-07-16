# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

include("openroad")

# ipl_lib -------------------------------------------------------------------
# The functional library: no GUI dependencies so it can be linked into
# standalone C++ unit tests without pulling in the full OpenROAD GUI stack.
add_library(ipl_lib
  src/IntegratedPlacer.cpp
)

target_include_directories(ipl_lib
  PUBLIC
    include
)

target_link_libraries(ipl_lib
  PUBLIC
    odb
  PRIVATE
    utl_lib
)

# ipl (swig target) ---------------------------------------------------------
# The application library: adds the Tcl/SWIG registration layer and any
# GUI-dependent code.  OpenROAD links against this target.
swig_lib(NAME      ipl
         NAMESPACE ipl
         I_FILE    src/ipl.i
         SCRIPTS   src/ipl.tcl
)

target_sources(ipl
  PRIVATE
    src/MakeIntegratedPlacer.cpp
)

target_include_directories(ipl
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(ipl
  PRIVATE
    ipl_lib
    utl_lib
)

messages(
  TARGET ipl
)

################################################################

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
