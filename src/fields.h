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

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

#include "./addr.h"
#include "./config.h"
#include "./constants.h"
#include "./util.h"

namespace spv {
enum class CCode : uint8_t {
  MALFORMED = 0x1,
  INVALID = 0x10,
  OBSOLETE = 0x11,
  DUPLICATE = 0x12,
  NONSTANDARD = 0x40,
  DUST = 0x41,
  INSUFFICIENT_FEE = 0x42,
  CHECKPOINT = 0x43,
};

const uint32_t WITNESS_FLAG = 1 << 30;

enum class InvType : uint32_t {
  ERROR = 0,
  TX = 1,
  BLOCK = 2,
  FILTERED_BLOCK = 3,
  CMPCT_BLOCK = 4,
  WITNESS_BLOCK = BLOCK | WITNESS_FLAG,
  WITNESS_TX = TX | WITNESS_FLAG,
  FILTERED_WITNESS_BLOCK = FILTERED_BLOCK | WITNESS_FLAG,
};

inline std::string to_string(const InvType &inv) {
  switch (inv) {
    case InvType::ERROR:
      return "ERROR";
    case InvType::TX:
      return "TX";
    case InvType::BLOCK:
      return "BLOCK";
    case InvType::FILTERED_BLOCK:
      return "FILTERED_BLOCK";
    case InvType::CMPCT_BLOCK:
      return "CMPCT_BLOCK";
    case InvType::WITNESS_BLOCK:
      return "WITNESS_BLOCK";
    case InvType::WITNESS_TX:
      return "WITNESS_TX";
    case InvType::FILTERED_WITNESS_BLOCK:
      return "FILTERED_WITNESS_BLOCK";
  }
  std::ostringstream os;
  os << "INV[" << static_cast<uint32_t>(inv) << "]";
  return os.str();
}

struct Inv {
  InvType type;
  hash_t hash;

  Inv() : type(InvType::ERROR), hash(empty_hash) {}
  Inv(InvType t, const hash_t &h) : type(t), hash(h) {}

  bool operator==(const Inv &other) const {
    return type == other.type && hash == other.hash;
  }
};

struct Headers {
  uint32_t magic;
  std::string command;
  uint32_t payload_size;
  uint32_t checksum;

  Headers() : magic(PROTOCOL_MAGIC), payload_size(0), checksum(0) {}
  explicit Headers(const std::string &command)
      : magic(PROTOCOL_MAGIC), command(command), payload_size(0), checksum(0) {}
  Headers(const Headers &other)
      : magic(other.magic),
        command(other.command),
        payload_size(other.payload_size),
        checksum(other.checksum) {}
};

struct BlockHeader {
  uint32_t version;  // supposed to be signed, but who cares
  hash_t prev_block;
  hash_t merkle_root;
  uint32_t timestamp;
  uint32_t bits;
  uint32_t nonce;

  // not encoded, for internal use only
  size_t height;
  hash_t block_hash;
  double accumulated_work;

  BlockHeader()
      : version(0),
        prev_block(empty_hash),
        merkle_root(empty_hash),
        timestamp(0),
        bits(0),
        nonce(0),
        height(0),
        block_hash(empty_hash),
        accumulated_work(0) {}
  BlockHeader(const BlockHeader &other)
      : version(other.version),
        prev_block(other.prev_block),
        merkle_root(other.merkle_root),
        timestamp(other.timestamp),
        bits(other.bits),
        nonce(other.nonce),
        height(other.height),
        block_hash(other.block_hash),
        accumulated_work(other.accumulated_work) {}

  static BlockHeader genesis();

  inline bool is_empty() const { return block_hash == empty_hash; }

  inline bool is_genesis() const {
    return block_hash == hash_t{0x00, 0x00, 0x00, 0x00, 0x09, 0x33, 0xea, 0x01,
                                0xad, 0x0e, 0xe9, 0x84, 0x20, 0x97, 0x79, 0xba,
                                0xae, 0xc3, 0xce, 0xd9, 0x0f, 0xa3, 0xf4, 0x08,
                                0x71, 0x95, 0x26, 0xf8, 0xd7, 0x7f, 0x49, 0x43};
  }

  inline bool is_orphan() const { return height == 0 && !is_genesis(); }

  // get the work required to mine this block
  double work_required() const;

  void daisy_chain(const BlockHeader &prev);

  // decode from db
  void db_decode(const std::string &s);

  // encode to db format
  std::string db_encode() const;

  // get the age of this header, in seconds
  uint32_t age() const;
};

struct VersionNetAddr {
  uint64_t services;
  Addr addr;

  VersionNetAddr() : services(0) {}
  VersionNetAddr(const VersionNetAddr &other)
      : services(other.services), addr(other.addr) {}
};

struct NetAddr {
  uint32_t time;
  uint64_t services;
  Addr addr;

  NetAddr() : time(time32()), services(0) {}
  NetAddr(const NetAddr &other)
      : time(other.time), services(other.services), addr(other.addr) {}

  inline bool operator==(const NetAddr &other) const {
    return addr == other.addr;
  }
};
}  // namespace spv

std::ostream &operator<<(std::ostream &o, const spv::BlockHeader &hdr);
std::ostream &operator<<(std::ostream &o, const spv::NetAddr &addr);

namespace std {
template <>
struct hash<spv::NetAddr> {
  std::size_t operator()(const spv::NetAddr &addr) const noexcept {
    return std::hash<spv::Addr>{}(addr.addr);
  }
};

template <>
struct hash<spv::Inv> {
  std::size_t operator()(const spv::Inv &inv) const noexcept {
    size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t>(inv.type));
    size_t h2 = std::hash<spv::hash_t>{}(inv.hash);
    return h1 ^ h2;
  }
};
}  // namespace std
