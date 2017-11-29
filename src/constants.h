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

namespace spv {
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

typedef std::array<uint8_t, 32> hash_t;
static_assert(sizeof(hash_t) == 32);

// hash of all zeros
const hash_t empty_hash{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// hash of the genesis block
const hash_t genesis_hash{0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0xd6, 0x68,
                          0x9c, 0x08, 0x5a, 0xe1, 0x65, 0x83, 0x1e, 0x93,
                          0x4f, 0xf7, 0x63, 0xae, 0x46, 0xa2, 0xa6, 0xc1,
                          0x72, 0xb3, 0xf1, 0xb6, 0x0a, 0x8c, 0xe2, 0x6f};

// merkle root of the genesis block
const hash_t genesis_root{0x4a, 0x5e, 0x1e, 0x4b, 0xaa, 0xb8, 0x9f, 0x3a,
                          0x32, 0x51, 0x8a, 0x88, 0xc3, 0x1b, 0xc8, 0x7f,
                          0x61, 0x8f, 0x76, 0x67, 0x3e, 0x2c, 0xc7, 0x7a,
                          0xb2, 0x12, 0x7b, 0x7a, 0xfd, 0xed, 0xa3, 0x3b};

}  // namespace spv
