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

#include "./encoder.h"

namespace spv {
template <>
void Encoder::push_int(uint16_t val) {
  val = htole16(val);
  buf_.copy(&val, sizeof val);
}

template <>
void Encoder::push_int(uint32_t val) {
  val = htole32(val);
  buf_.copy(&val, sizeof val);
}

template <>
void Encoder::push_int(uint64_t val) {
  val = htole64(val);
  buf_.copy(&val, sizeof val);
}

void Encoder::push_varint(size_t val) {
  if (val < 0xfd) {
    push_byte(val);
    return;
  } else if (val <= 0xffff) {
    push_byte(0xfd);
    push_int<uint16_t>(val);
    return;
  } else if (val <= 0xffffffff) {
    push_byte(0xfe);
    push_int<uint32_t>(val);
    return;
  }
  push_byte(0xff);
  push_int<uint64_t>(val);
}

void Encoder::push_netaddr(const Addr *addr, uint64_t services,
                           bool include_time) {
  if (include_time) {
    push_time<uint32_t>();
  }
  push_int<uint64_t>(services);
}

#if 0
void Encoder::push_addr(const NetAddr *addr) {
  if (addr == nullptr) {
    push_bytes(reinterpret_cast<const void *>(&empty_netaddr),
               sizeof empty_netaddr);
    return;
  }
  push_int<uint32_t>(addr->time);
  push_int<uint64_t>(addr->services);
  push_bytes(reinterpret_cast<const void *>(&addr->addr), sizeof addr->addr);
  push_int<uint16_t>(addr->port);
}
#endif
}  // namespace spv
