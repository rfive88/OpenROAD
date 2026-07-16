// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Metrics aggregates instance-count and area statistics for a logical
// module or a physical cluster.  It is used by ClusteringEngine to
// decide when to merge or split clusters (see Hier-RTLMP §III-B).
// All areas are in OpenDB database units squared (dbu²).

#pragma once

#include <cstdint>

namespace ipl {

class Metrics
{
 public:
  Metrics() = default;
  Metrics(unsigned int num_std_cell,
          unsigned int num_macro,
          int64_t std_cell_area,
          int64_t macro_area);

  // Accumulate another Metrics into this one (used when merging clusters).
  void add(const Metrics& other);

  unsigned int getNumStdCell() const { return num_std_cell_; }
  unsigned int getNumMacro() const { return num_macro_; }
  int64_t getStdCellArea() const { return std_cell_area_; }
  int64_t getMacroArea() const { return macro_area_; }
  int64_t getTotalArea() const { return std_cell_area_ + macro_area_; }

  bool empty() const { return num_std_cell_ == 0 && num_macro_ == 0; }

 private:
  unsigned int num_std_cell_{0};
  unsigned int num_macro_{0};
  int64_t std_cell_area_{0};  // dbu²
  int64_t macro_area_{0};     // dbu²
};

}  // namespace ipl
