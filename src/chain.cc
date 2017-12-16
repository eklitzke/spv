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
#include <unordered_map>

#include "./encoder.h"
#include "./logging.h"

namespace spv {
MODULE_LOGGER

static const rocksdb::ReadOptions read_opts;
static const rocksdb::WriteOptions write_opts;

static const std::string tip_key = "tip";

// If this block is at a checkpointed height, verify that we have the expected
// block hash.
inline void check_checkpoint(const BlockHeader &hdr) {
  static const size_t checkpoint_interval = 500000;
  static const std::unordered_map<size_t, hash_t> checkpoints{
      {500000,
       {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xa7, 0xc0, 0xaa, 0xa2, 0x63,
        0x0f, 0xbb, 0x2c, 0x0e, 0x47, 0x6a, 0xaf, 0xff, 0xc6, 0x0f, 0x82,
        0x17, 0x73, 0x75, 0xb2, 0xaa, 0xa2, 0x22, 0x09, 0xf6, 0x06}},
      {1000000,
       {0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x8e, 0x25, 0x9a, 0x3e, 0xda,
        0x2f, 0xaf, 0xbe, 0xeb, 0x01, 0x06, 0x62, 0x6f, 0x94, 0x63, 0x47,
        0x95, 0x5e, 0x99, 0x27, 0x8f, 0xe6, 0xcc, 0x84, 0x84, 0x14}},
  };
  if (hdr.height % checkpoint_interval == 0) {
    auto it = checkpoints.find(hdr.height);
    assert(it != checkpoints.end());
    assert(hdr.block_hash == it->second);
  }
}

inline std::string encode_key(hash_t block_hash) {
  std::reverse(block_hash.begin(), block_hash.end());  // n.b. by value
  return {reinterpret_cast<const char *>(block_hash.data()), sizeof(hash_t)};
}

Chain::Chain(const std::string &datadir) {
  rocksdb::Options dbopts;
  dbopts.OptimizeForSmallDb();
  auto status = rocksdb::DB::Open(dbopts, datadir, &db_);
  if (status.ok()) {
    tip_ = find_tip();
    log->info("initialized with at {}", tip_);
    return;
  }

  dbopts.create_if_missing = true;
  dbopts.error_if_exists = true;
  status = rocksdb::DB::Open(dbopts, datadir, &db_);
  assert(status.ok());
  update_database(BlockHeader::genesis());
}

void Chain::update_database(const BlockHeader &hdr) {
  if (!tip_.height || hdr.height > tip_.height) {
    tip_ = hdr;
  }

  const std::string key = encode_key(hdr.block_hash);
  const std::string val = hdr.db_encode();
  log->debug("writing {} bytes for block {}", val.size(), hdr);
  auto s = db_->Put(write_opts, key, val);
  assert(s.ok());
}

rocksdb::Status Chain::find_block_header(BlockHeader &hdr) {
  const std::string key = encode_key(hdr.block_hash);
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
  prev_block.block_hash = hdr.prev_block;
  auto s = find_block_header(prev_block);
  assert(s.ok());

  BlockHeader copy(hdr);
  copy.height = prev_block.height + 1;
  update_database(copy);
  check_checkpoint(copy);
}

void Chain::save_tip() {
  auto s = db_->Put(write_opts, tip_key, tip_.db_encode());
  assert(s.ok());
}
}  // namespace spv
