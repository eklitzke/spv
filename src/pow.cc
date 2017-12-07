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

#include "./pow.h"

#include <endian.h>
#include <cstring>

#include "picosha2/picosha2.h"

namespace spv {
hash_t pow_hash(const char *data, size_t sz, bool big_endian) {
  hash_t hash1, hash2;
  picosha2::hash256(data, data + sz, hash1);
  picosha2::hash256(hash1, hash2);

  // XXX: technically we should only call this if we know we're on a LE host
  if (__BYTE_ORDER == __LITTLE_ENDIAN && big_endian) {
    std::reverse(hash2.begin(), hash2.end());
  }

  return hash2;
}

void checksum(const char *data, size_t sz, std::array<char, 4> &out) {
  hash_t hash = pow_hash(data, sz);
  std::memcpy(out.data(), hash.data(), 4);
}

uint32_t checksum(const char *data, size_t sz) {
  std::array<char, 4> arr;
  checksum(data, sz, arr);
  uint32_t out;
  std::memmove(&out, arr.data(), sizeof out);
  return out;
}
}  // namespace spv
