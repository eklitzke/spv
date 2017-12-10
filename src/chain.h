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
  Chain() = delete;
  Chain(const Chain &other) = delete;
  ~Chain() {
    save_tip();
    db_.reset();
  }

  // add a block header
  void put_block_header(const BlockHeader &hdr, bool check_duplicate = true);

  // save the tip
  void save_tip();

  inline const BlockHeader &tip() const { return tip_; }

 private:
  std::unique_ptr<rocksdb::DB> db_;
  BlockHeader tip_;

  void update_database(const BlockHeader &hdr);

  rocksdb::Status find_block_header(BlockHeader &hdr);
  BlockHeader find_tip();

 protected:
  Chain(const std::string &datadir);
};
}  // namespace spv
