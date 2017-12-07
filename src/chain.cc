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

#include <cassert>

#include "./logging.h"

namespace spv {
MODULE_LOGGER

void Chain::add_block(const BlockHeader &hdr) {
  auto it = headers_.find(hdr.prev_block);
  assert(it != headers_.end());

  BlockHeader copy(hdr);
  copy.height = it->second.height + 1;
  auto pr = headers_.emplace(std::make_pair(copy.block_hash, copy));
  log->debug("added block {} at height {} to chain", hdr, copy.height);

  if (pr.second && copy.height > tip_->height) {
    tip_ = &pr.first->second;
    log->info("new chain tip at height {}: {}", copy.height, copy);
  }
}
}  // namespace spv
