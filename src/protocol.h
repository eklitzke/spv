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

#include <cstdint>
#include <cstring>
#include <string>

#include "./addr.h"
#include "./util.h"

namespace spv {
// these must be unsigned, enums are signed
inline uint32_t MAINNET_MAGIC = 0xD9B4BEF9;
inline uint32_t TESTNET_MAGIC = 0xDAB5BFFA;
inline uint32_t TESTNET3_MAGIC = 0x0709110B;

// constants related to the protocol itself
enum {
  PROTOCOL_VERSION = 70015,
  TESTNET_PORT = 18333,
};

// constants related to message headers
enum {
  HEADER_PADDING = 8,
  HEADER_SIZE = 24,
  HEADER_LEN_OFFSET = 16,
  HEADER_CHECKSUM_OFFSET = 20,
};

// netaddr constants
enum {
  ADDR_SIZE = 16,
  PORT_SIZE = 2,
};

// constants related to version messages
enum {
  COMMAND_SIZE = 12,
};

struct Headers {
  uint32_t magic;
  std::string command;
  uint32_t payload_size;
  uint32_t checksum;

  Headers() : magic(TESTNET3_MAGIC), payload_size(0), checksum(0) {}
  explicit Headers(const std::string &command)
      : magic(TESTNET3_MAGIC), command(command), payload_size(0), checksum(0) {}
  Headers(const Headers &other)
      : magic(other.magic),
        command(other.command),
        payload_size(other.payload_size),
        checksum(other.checksum) {}
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

  virtual std::unique_ptr<char[]> encode(size_t &sz) const = 0;
};

#define FINAL_ENCODE std::unique_ptr<char[]> encode(size_t &sz) const final;

struct Ping : Message {
  uint64_t nonce;

  Ping() : Ping(0) {}
  Ping(uint64_t nonce) : Message("ping"), nonce(nonce) {}
  FINAL_ENCODE
};

struct Pong : Message {
  uint64_t nonce;

  Pong() : Pong(0) {}
  Pong(uint64_t nonce) : Message("pong"), nonce(nonce) {}
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

  Version()
      : Message("version"),
        version(0),
        services(0),
        timestamp(time64()),
        nonce(0),
        start_height(0),
        relay(0) {}
  FINAL_ENCODE
};

struct Verack : Message {
  Verack() : Message("verack") {}
  FINAL_ENCODE
};
}  // namespace spv
