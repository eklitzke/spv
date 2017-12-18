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

rocksdb::ReadOptions read_opts;
rocksdb::WriteOptions write_opts;

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

Chain::Chain(const std::string &datadir)
    : hdr_view_('h'), orphan_view_('o'), height_view_('y') {
  rocksdb::Options dbopts;
  dbopts.OptimizeForSmallDb();
  auto status = rocksdb::DB::Open(dbopts, datadir, &db_);
  if (status.ok()) {
    initialize_views();
    tip_ = find_tip();
    log->info("initialized chain with tip {}", tip_);
    return;
  }

  dbopts.create_if_missing = true;
  dbopts.error_if_exists = true;
  status = rocksdb::DB::Open(dbopts, datadir, &db_);
  assert(status.ok());
  initialize_views();
  add_genesis_block();
}

void Chain::add_genesis_block() {
  // TODO: use a transaction
  tip_ = BlockHeader::genesis();
  assert(hdr_view_.put(tip_.block_hash, tip_.db_encode()));
  assert(height_view_.put(0, tip_.block_hash));
  save_tip();
}

BlockHeader Chain::find(const hash_t &hash) const {
  bool found = false;
  const std::string data = hdr_view_.find(hash, found);
  assert(found);

  BlockHeader hdr;
  hdr.db_decode(data);
  return hdr;
}

BlockHeader Chain::find_tip() {
  std::string val;
  auto s = db_->Get(read_opts, tip_key, &val);
  if (!s.ok()) {
    log->warn("no tip...");
    add_genesis_block();
    s = db_->Get(read_opts, tip_key, &val);
  }
  assert(s.ok());
  const hash_t tip_hash = decode_hash(val);
  log->debug("fetching tip whose hash is {}", tip_hash);
  return find(tip_hash);
}

void Chain::put_block_header(const BlockHeader &hdr, bool check_duplicate) {
  assert(hdr.block_hash != empty_hash);
  bool found = false;
  std::string prev_block_data = hdr_view_.find(hdr.prev_block, found);
  if (found) {
    BlockHeader prev_block;
    prev_block.db_decode(prev_block_data);
    if (prev_block.is_genesis() || prev_block.height) {
      // insert the block with the correct block height
      BlockHeader copy(hdr);
      copy.height = prev_block.height + 1;
      check_checkpoint(copy);
      assert(hdr_view_.put(copy.block_hash, copy.db_encode()));
      assert(height_view_.put(copy.height, copy.block_hash));
      attach_orphan(copy);
      update_tip(copy);
      return;
    }
  }

  // This is an orphan block; either the ancestor doesn't exist, or the ancestor
  // is an orphan. This is indexed based on the orphan's prev_block;
  assert(orphan_view_.put(hdr.prev_block, hdr.db_encode()));
  log->debug("added orphan block {}", hdr);
}

bool Chain::attach_orphan(const BlockHeader &hdr) {
  assert(hdr.height || hdr.is_genesis());

  bool found = false;
  const std::string data = orphan_view_.find(hdr.block_hash, found);
  if (!found) {
    return false;
  }

  BlockHeader orphan;
  orphan.db_decode(data);
  assert(orphan.height == 0);
  assert(orphan.prev_block == hdr.block_hash);
  orphan.height = hdr.height + 1;

  // TODO: Use a tx for this.
  // TODO: Handle the case where multiple block have the same height.
  assert(hdr_view_.put(orphan.block_hash, orphan.db_encode()));
  assert(height_view_.put(orphan.height, orphan.block_hash));
  assert(orphan_view_.erase(hdr.block_hash));
  log->warn("attached orphan {}", orphan);

  update_tip(orphan);

  // look for orphans recursively
  attach_orphan(orphan);
  return true;
}

void Chain::update_tip(const BlockHeader &hdr) {
  if (hdr.height < tip_.height) {
    return;
  }
  assert(hdr.height == tip_.height + 1);
  tip_ = hdr;
}

bool Chain::save_tip(bool check) {
  if (check) {
    assert(!tip_.is_empty());
    assert(!tip_.is_orphan());
  }
  auto s = db_->Put(write_opts, tip_key, encode_hash(tip_.block_hash));
  log->debug("saved chain tip {}", tip_);
  if (check) {
    assert(s.ok());
  }
  return s.ok();
}
}  // namespace spv
