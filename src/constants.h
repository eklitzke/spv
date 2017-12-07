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

// the raw testnet genesis block header data
const std::array<uint8_t, 80> genesis_block_hdr{
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2, 0x7a, 0xc7, 0x2c, 0x3e,
    0x67, 0x76, 0x8f, 0x61, 0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
    0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a, 0xda, 0xe5, 0x49, 0x4d,
    0xff, 0xff, 0x00, 0x1d, 0x1a, 0xa4, 0xae, 0x18};
}  // namespace spv

std::ostream &operator<<(std::ostream &o, const spv::hash_t &h);

namespace std {
template <>
struct hash<spv::hash_t> {
  std::size_t operator()(const spv::hash_t &input) const noexcept {
    std::size_t h = 0;
    const size_t *data = reinterpret_cast<const size_t *>(input.data());
    for (size_t i = 0; i < sizeof(spv::hash_t) / sizeof(size_t); i++) {
      h |= std::hash<size_t>{}(*(data + i));
    }
    return h;
  }
};
}  // namespace std
