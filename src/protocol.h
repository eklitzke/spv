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

struct Headers {
  uint32_t magic;
  char command[12];
  uint32_t payload_size;
  uint32_t checksum;
};
static_assert(sizeof(Headers) == HEADER_SIZE);

// netaddr constants
enum {
  ADDR_SIZE = 16,
  PORT_SIZE = 2,
};

// constants related to version messages
enum {
  COMMAND_SIZE = 12,
};
}  // namespace spv
