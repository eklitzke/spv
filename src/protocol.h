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

#include "./uvw.h"

namespace spv {
// these must be unsigned, enums are signed
#define MAINNET_MAGIC 0xD9B4BEF9
#define TESTNET_MAGIC 0xDAB5BFFA
#define TESTNET3_MAGIC 0x0709110B

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

  Headers() : magic(0), payload_size(0), checksum(0) {}
  explicit Headers(const std::string &command)
      : magic(0), command(command), payload_size(0), checksum(0) {}
  Headers(const Headers &other)
      : magic(other.magic),
        command(other.command),
        payload_size(other.payload_size),
        checksum(other.checksum) {}
};

#define VERSION_FIELDS \
  uint64_t services;   \
  char addr[16];       \
  uint16_t port;

struct VersionNetAddr {
  VERSION_FIELDS
};

struct NetAddr {
  uint32_t time;
  VERSION_FIELDS
};

enum {
  VERSION_NETADDR_SIZE = 26,
  NETADDR_SIZE = 30,
};

struct Message {
  Headers headers;

  Message() = delete;
  Message(const Headers &hdrs) : headers(hdrs) {}
};

struct Ping : Message {
  uint64_t nonce;
};

struct Pong : Message {
  uint64_t nonce;
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

  Version() = delete;
  Version(const Headers &hdrs) : Message(hdrs) {}
};

struct Verack : Message {
  Verack() = delete;
  Verack(const Headers &hdrs) : Message(hdrs) {}
};
}  // namespace spv
