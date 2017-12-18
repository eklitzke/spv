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
#include <rocksdb/utilities/optimistic_transaction_db.h>

#include "./fields.h"

namespace spv {
class Client;
class Chain;

extern rocksdb::ReadOptions read_opts;
extern rocksdb::WriteOptions write_opts;

inline std::string encode_hash(hash_t hash) {  // by value
  std::reverse(hash.begin(), hash.end());
  return {reinterpret_cast<const char *>(hash.data()), sizeof(hash_t)};
}

inline hash_t decode_hash(const std::string &val) {
  assert(val.size() == sizeof(hash_t));
  hash_t out;
  std::memmove(out.data(), val.data(), sizeof(hash_t));
  std::reverse(out.begin(), out.end());
  return out;
}

class TableView {
  friend class Chain;

 public:
  TableView() = delete;
  explicit TableView(char prefix) : db_(nullptr), prefix_(prefix) {}
  TableView(rocksdb::DB *db, char prefix) : db_(db), prefix_(prefix) {}

  inline bool has_key(const hash_t &hash) const {
    std::string val;
    auto s = db_->Get(read_opts, encode_key(hash), &val);
    return s.ok();
  }

  inline std::string find(const std::string &key, bool &found) const {
    std::string val;
    auto s = db_->Get(read_opts, key, &val);
    found = s.ok();
    return val;
  }

  inline std::string find(size_t height, bool &found) const {
    return find(encode_key(height), found);
  }

  inline std::string find(const hash_t &hash, bool &found) const {
    return find(encode_key(hash), found);
  }

  inline bool erase(const hash_t &hash) {
    auto s = db_->Delete(write_opts, encode_key(hash));
    return s.ok();
  }

  inline bool put(const std::string &key, const std::string &val) {
    auto s = db_->Put(write_opts, key, val);
    return s.ok();
  }

  inline bool put(const hash_t &hash, const std::string &data) {
    assert(hash != empty_hash);
    return put(encode_key(hash), data);
  }

  inline bool put(size_t height, const hash_t &hash) {
    return put(encode_key(height), encode_key(hash));
  }

 private:
  rocksdb::DB *db_;
  char prefix_;

  inline std::string encode_key(const hash_t &hash) const {
    return prefix_ + encode_hash(hash);
  }

  inline std::string encode_key(size_t height) const {
    std::ostringstream os;
    os << prefix_ << height;
    return os.str();
  }

 protected:
  void set_db(rocksdb::DB *db) {
    assert(db_ == nullptr);
    db_ = db;
  }
};

class Chain {
  friend Client;

 public:
  Chain() = delete;
  Chain(const Chain &other) = delete;
  ~Chain() { save_tip(true); }

  // add a block header
  void put_block_header(const BlockHeader &hdr, bool check_duplicate = true);

  // save the tip
  bool save_tip(bool check = true);

  // is the tip recent?
  inline bool tip_is_recent(uint32_t seconds_cutoff = 3600) const {
    return tip_.age() < seconds_cutoff;
  }

  inline const BlockHeader &tip() const { return tip_; }

  inline size_t height() const { return tip_.height; }

  inline bool has_block(const hash_t &hash) const {
    return hdr_view_.has_key(hash) || orphan_view_.has_key(hash);
  }

  BlockHeader find(const hash_t &hash) const;

 private:
  // N.B. There's a lot of RocksDB stuff in valgrind when code shuts down via a
  // signal handler. This should be a raw pointer because RocksDB somehow
  // atuomatically deletes any open DB handles, but the code here needs to be
  // cleaned up to clearnly pass valgrind.
  rocksdb::DB *db_;
  rocksdb::Transaction *tx_;

  // The tip of the blockchain
  BlockHeader tip_;

  // TODO: this could benefit from some caching. We probably want to cache the
  // last N non-orphan headers (where N could be pretty small, possibly even
  // just 1) and probably all of the orphans.
  TableView hdr_view_;
  TableView orphan_view_;
  TableView height_view_;

  void add_genesis_block();

  // Get the block at the tip.
  BlockHeader find_tip();

  // If there is an orphan of this header, attach it.
  bool attach_orphan(const BlockHeader &hdr);

  // Try to update the tip.
  void update_tip(const BlockHeader &hdr);

  inline void initialize_views() {
    assert(db_ != nullptr);
    hdr_view_.set_db(db_);
    orphan_view_.set_db(db_);
    height_view_.set_db(db_);
  }

 protected:
  Chain(const std::string &datadir);
};
}  // namespace spv
