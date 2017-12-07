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

#include "./chain.h"

namespace spv {
void Chain::add_child(const BlockHeader &hdr) {
  // this is kind of gross
  std::unique_ptr<Chain> child(new Chain(hdr, hdr_.height + 1));
  children_.emplace_back(std::move(child));
}

BlockHeader Chain::tip() const {
  BlockHeader best_hdr = hdr_;
  for (const auto &child : children_) {
    child->tip_helper(best_hdr);
  }
  return best_hdr;
}

void Chain::tip_helper(BlockHeader &hdr) const {
  if (hdr_.height > hdr.height) {
    hdr = hdr_;
  }
}
}  // namespace spv
