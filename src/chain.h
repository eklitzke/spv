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

#include <rocksdb/db.h>
#include <memory>

#include "./fields.h"

namespace spv {
class Client;

class Chain {
  friend Client;

 public:
  Chain(const Chain &other) = delete;

  void put_block_header(const BlockHeader &hdr, bool check_duplicate = true);

  inline size_t tip_height() const { return tip_height_; }
  inline const hash_t &tip_hash() const { return tip_hash_; }

 private:
  std::unique_ptr<rocksdb::DB> db_;
  size_t tip_height_;
  hash_t tip_hash_;

  void update_database(const BlockHeader &hdr);

  rocksdb::Status find_block_header(const hash_t &block_hash, BlockHeader &hdr);
  BlockHeader find_tip();

 protected:
  Chain();
};
}  // namespace spv
