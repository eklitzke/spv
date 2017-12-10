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
#include <string>

#include "./encoder.h"
#include "./logging.h"

namespace spv {
MODULE_LOGGER

static const rocksdb::ReadOptions read_opts;
static const rocksdb::WriteOptions write_opts;

static const std::string tip_key = "tip";

inline std::string encode_key(const hash_t &block_hash) {
  return "h" + std::string(reinterpret_cast<const char *>(block_hash.data()),
                           sizeof(hash_t));
}

Chain::Chain(const std::string &datadir) {
  rocksdb::Options options;
  rocksdb::DB *db;
  auto status = rocksdb::DB::Open(options, datadir, &db);
  if (status.ok()) {
    db_.reset(db);
    tip_ = find_tip();
    log->info("initialized with at {}", tip_);
    return;
  }

  options.create_if_missing = true;
  status = rocksdb::DB::Open(options, datadir, &db);
  assert(status.ok());
  db_.reset(db);
  update_database(BlockHeader::genesis());
}

void Chain::update_database(const BlockHeader &hdr) {
  if (!tip_.height || hdr.height > tip_.height) {
    tip_ = hdr;
  }

  const std::string key = encode_key(hdr.block_hash);
  const std::string val = hdr.db_encode();
  log->debug("writing {} bytes for genesis block", val.size());
  auto s = db_->Put(write_opts, key, val);
  assert(s.ok());
}

rocksdb::Status Chain::find_block_header(const hash_t &block_hash,
                                         BlockHeader &hdr) {
  const std::string key = encode_key(block_hash);
  std::string val;
  auto s = db_->Get(read_opts, key, &val);
  if (!s.ok()) {
    return s;
  }
  assert(val.size());
  hdr.db_decode(val);
  return s;
}

BlockHeader Chain::find_tip() {
  std::string val;
  auto s = db_->Get(read_opts, tip_key, &val);
  assert(s.ok());

  BlockHeader hdr;
  hdr.db_decode(val);
  return hdr;
}

void Chain::put_block_header(const BlockHeader &hdr, bool check_duplicate) {
  log->debug("putting block header {}", hdr);
  std::string key = encode_key(hdr.block_hash);

  if (check_duplicate) {
    std::string value;
    auto s = db_->Get(read_opts, key, &value);
    if (s.ok()) {
      log->warn("ignoring duplicate header {}", hdr);
      return;
    }
  }

  BlockHeader prev_block;
  auto s = find_block_header(hdr.prev_block, prev_block);
  assert(s.ok());

  BlockHeader copy(hdr);
  copy.height = prev_block.height + 1;
  update_database(copy);
}

void Chain::save_tip() {
  auto s = db_->Put(write_opts, tip_key, tip_.db_encode());
  assert(s.ok());
}
}  // namespace spv
