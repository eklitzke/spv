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

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "./addr.h"
#include "./config.h"
#include "./constants.h"
#include "./util.h"

namespace spv {
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
  uint8_t tx_count;  // should always be zero

  BlockHeader()
      : version(0),
        prev_block(empty_hash),
        merkle_root(empty_hash),
        timestamp(0),
        difficulty(0),
        nonce(0),
        tx_count(0) {}
};

struct VersionNetAddr {
  uint64_t services;
  Addr addr;

  VersionNetAddr() : services(0) {}
};

struct NetAddr {
  uint32_t time;
  uint64_t services;
  Addr addr;

  NetAddr() : time(time32()), services(0) {}
};

struct Message {
  Headers headers;

  Message() {}
  explicit Message(const std::string &command) : headers(command) {}
  explicit Message(const Headers &hdrs) : headers(hdrs) {}

  virtual std::unique_ptr<char[]> encode(size_t &sz) const = 0;
};

#define FINAL_ENCODE std::unique_ptr<char[]> encode(size_t &sz) const final;

struct AddrMsg : Message {
  std::vector<NetAddr> addrs;

  AddrMsg() : AddrMsg(Headers("addr")) {}
  explicit AddrMsg(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct GetAddr : Message {
  GetAddr() : GetAddr(Headers("getaddr")) {}
  explicit GetAddr(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct GetBlocks : Message {
  uint32_t version;
  size_t hash_count;
  std::vector<hash_t> block_locators;
  hash_t hash_stop;

  GetBlocks() : GetBlocks(Headers("getblocks")) {}
  explicit GetBlocks(const Headers &hdrs)
      : Message(hdrs), version(0), hash_count(0), hash_stop(empty_hash) {}
  FINAL_ENCODE
};

struct GetHeaders : Message {
  uint32_t version;
  size_t hash_count;
  std::vector<hash_t> block_locators;
  hash_t hash_stop;

  GetHeaders() : GetHeaders(Headers("getheaders")) {}
  explicit GetHeaders(const Headers &hdrs)
      : Message(hdrs), version(0), hash_count(0), hash_stop(empty_hash) {}
  FINAL_ENCODE
};

struct Ping : Message {
  uint64_t nonce;

  Ping() : Ping(Headers("ping")) {}
  explicit Ping(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct Pong : Message {
  uint64_t nonce;

  Pong() : Pong(Headers("pong")) {}
  explicit Pong(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct SendHeaders : Message {
  SendHeaders() : SendHeaders(Headers("sendheaders")) {}
  explicit SendHeaders(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct Version : Message {
  uint32_t version;
  uint64_t services;
  uint64_t timestamp;
  VersionNetAddr addr_recv;
  VersionNetAddr addr_from;
  uint64_t nonce;
  std::string user_agent;
  uint32_t start_height;
  uint8_t relay;

  Version() : Version(Headers("version")) {}
  explicit Version(const Headers &hdrs)
      : Message(hdrs),
        version(0),
        services(0),
        timestamp(time64()),
        nonce(0),
        start_height(0),
        relay(0) {}
  FINAL_ENCODE
};

struct VerAck : Message {
  VerAck() : VerAck(Headers("verack")) {}
  explicit VerAck(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

std::unique_ptr<Message> decode_message(const char *data, size_t size,
                                        size_t *bytes_consumed);
}  // namespace spv
