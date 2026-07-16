// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Metrics implementation: member-wise construction and accumulation.

#include "metrics.h"

#include <cstdint>

namespace ipl {

Metrics::Metrics(unsigned int num_std_cell,
                 unsigned int num_macro,
                 int64_t std_cell_area,
                 int64_t macro_area)
    : num_std_cell_(num_std_cell),
      num_macro_(num_macro),
      std_cell_area_(std_cell_area),
      macro_area_(macro_area)
{
}

// add -----------------------------------------------------------------------
// Accumulates the counts and areas of another Metrics into this one.  Called
// when clusters are merged, and when ClusteringEngine rolls child module
// metrics up into their parent.
void Metrics::add(const Metrics& other)
{
  num_std_cell_ += other.num_std_cell_;
  num_macro_ += other.num_macro_;
  std_cell_area_ += other.std_cell_area_;
  macro_area_ += other.macro_area_;
}

}  // namespace ipl
