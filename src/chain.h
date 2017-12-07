// Copyright (c) 2017 Evan Klitzke <evan@eklitzke.org>
//
// This file is part of SPV.
//
// SPV is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// SPV is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// SPV. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <memory>

#include "./fields.h"

namespace spv {
class Chain {
 public:
  Chain() : height_(0) {}
  Chain(size_t height, const BlockHeader &hdr) : height_(height), hdr_(hdr) {}

  inline bool empty() const { return height_ == 0 && hdr_.empty(); }

  void set_hdr(const BlockHeader &hdr) { hdr_ = hdr; }

  void add_child(const BlockHeader &hdr) {
    // children_.emplace_back(height_ + 1, hdr);
  }

 private:
  size_t height_;
  BlockHeader hdr_;
  std::vector<std::unique_ptr<Chain>> children_;
};
}  // namespace spv
