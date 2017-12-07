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

#include <cassert>
#include <unordered_map>

#include "./fields.h"

namespace spv {
class Client;

class Chain {
  friend Client;

 public:
  Chain(const Chain &other) = delete;

  void add_block(const BlockHeader &hdr);

  const BlockHeader *tip() const { return tip_; }

 private:
  std::unordered_map<hash_t, BlockHeader> headers_;
  BlockHeader *tip_;

 protected:
  Chain() {
    auto genesis = BlockHeader::genesis();
    auto pr = headers_.emplace(genesis.block_hash, genesis);
    assert(pr.second);
    tip_ = &pr.first->second;
  }
};
}  // namespace spv
