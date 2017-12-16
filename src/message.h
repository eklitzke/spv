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
#include "./fields.h"
#include "./util.h"

namespace spv {
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

#if 0
  // need to add Tx
struct Block : Message {
  uint32_t version;
  hash_t prev_block;
  hash_t merkle_root;
  uint32_t timestamp;
  uint32_t difficulty;
  uint32_t nonce;
  std::vector<Tx> txns;

  Block() : Block(Headers("block")) {}
  explicit Block(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};
#endif

struct GetAddr : Message {
  GetAddr() : GetAddr(Headers("getaddr")) {}
  explicit GetAddr(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct GetBlocks : Message {
  uint32_t version;
  std::vector<hash_t> locator_hashes;
  hash_t hash_stop;

  GetBlocks() : GetBlocks(Headers("getblocks")) {}
  explicit GetBlocks(const Headers &hdrs)
      : Message(hdrs), version(0), hash_stop(empty_hash) {}
  FINAL_ENCODE
};

struct GetHeaders : Message {
  uint32_t version;
  std::vector<hash_t> locator_hashes;
  hash_t hash_stop;

  GetHeaders() : GetHeaders(Headers("getheaders")) {}
  explicit GetHeaders(const Headers &hdrs)
      : Message(hdrs), version(0), hash_stop(empty_hash) {}
  FINAL_ENCODE
};

struct HeadersMsg : Message {
  std::vector<BlockHeader> block_headers;

  HeadersMsg() : HeadersMsg(Headers("headers")) {}
  explicit HeadersMsg(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct Inv : Message {
  std::vector<std::pair<InvType, hash_t>> invs;

  Inv() : Inv(Headers("inv")) {}
  explicit Inv(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct Mempool : Message {
  Mempool() : Mempool(Headers("mempool")) {}
  explicit Mempool(const Headers &hdrs) : Message(hdrs) {}
  FINAL_ENCODE
};

struct Ping : Message {
  uint64_t nonce;

  Ping() : Ping(Headers("ping")) {}
  explicit Ping(const Headers &hdrs) : Message(hdrs), nonce(0) {}
  FINAL_ENCODE
};

struct Pong : Message {
  uint64_t nonce;

  Pong() : Pong(Headers("pong")) {}
  explicit Pong(const Headers &hdrs) : Message(hdrs), nonce(0) {}
  FINAL_ENCODE
};

struct Reject : Message {
  std::string message;
  CCode ccode;
  std::string reason;
  hash_t data;  // optional

  Reject() : Reject(Headers("reject")) {}
  explicit Reject(const Headers &hdrs) : Message(hdrs) {}
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
