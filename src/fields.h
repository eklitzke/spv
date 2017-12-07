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

enum class InvType : uint32_t {
  ERROR = 0,
  TX = 1,
  BLOCK = 2,
  FILTERED_BLOCK = 3,
  CMPCT_BLOCK = 4,
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
  uint32_t difficulty;
  uint32_t nonce;

  // not encoded, for internal use only
  size_t height;
  hash_t block_hash;

  BlockHeader()
      : version(0),
        prev_block(empty_hash),
        merkle_root(empty_hash),
        timestamp(0),
        difficulty(0),
        nonce(0),
        height(0),
        block_hash(empty_hash) {}
  BlockHeader(const BlockHeader &other)
      : version(other.version),
        prev_block(other.prev_block),
        merkle_root(other.merkle_root),
        timestamp(other.timestamp),
        difficulty(other.difficulty),
        nonce(other.nonce),
        height(other.height),
        block_hash(other.block_hash) {}

  // implemented in decoders.cc
  static BlockHeader genesis();

  // encode as a protobuf
  std::string to_proto() const;
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
};
}  // namespace spv

std::ostream &operator<<(std::ostream &o, const spv::BlockHeader &hdr);
