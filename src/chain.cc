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

static const std::string tip_key = "tip-hash";

inline std::string encode_key(const hash_t &block_hash) {
  return "hdr-" + std::string(reinterpret_cast<const char *>(block_hash.data()),
                              sizeof(hash_t));
}

Chain::Chain() : tip_height_(0), tip_hash_(empty_hash) {
  rocksdb::Options options;
  rocksdb::DB *db;
  auto status = rocksdb::DB::Open(options, ".spv", &db);
  if (status.ok()) {
    db_.reset(db);
    BlockHeader hdr = find_tip();
    tip_height_ = hdr.height;
    tip_hash_ = hdr.block_hash;
    log->info("initialized with chain height {}", tip_height_);
    return;
  }

  options.create_if_missing = true;
  status = rocksdb::DB::Open(options, ".spv", &db);
  assert(status.ok());
  db_.reset(db);
  update_database(BlockHeader::genesis());
}

void Chain::update_database(const BlockHeader &hdr) {
  if (!tip_height_ || hdr.height > tip_height_) {
    tip_height_ = hdr.height;
    tip_hash_ = hdr.block_hash;

    // XXX: this is kind of ghetto
    auto s = db_->Put(write_opts, tip_key, hdr.hash_str());
    assert(s.ok());
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

  hash_t tip_hash;
  assert(sizeof(hash_t) == val.size());
  std::memmove(tip_hash.data(), val.data(), sizeof(hash_t));

  BlockHeader hdr;
  s = find_block_header(tip_hash, hdr);
  assert(s.ok());
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
}  // namespace spv
