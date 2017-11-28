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

#include <array>

#include "./logging.h"
#include "./pow.h"

namespace spv {
MODULE_LOGGER
template <>
void Encoder::push_int(uint16_t val) {
  val = htole16(val);
  append(&val, sizeof val);
}

template <>
void Encoder::push_int(uint32_t val) {
  val = htole32(val);
  append(&val, sizeof val);
}

template <>
void Encoder::push_int(uint64_t val) {
  val = htole64(val);
  append(&val, sizeof val);
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

  if (addr == nullptr) {
    append_zeros(ADDR_SIZE + PORT_SIZE);
    return;
  }

  const int af = addr->af();
  switch (af) {
    case AF_INET:
      push_ipv4(addr->ipv4());
      break;
    case AF_INET6:
      push_ipv6(addr->ipv6());
      break;
    default:
      log->error("unknown address family {}", af);
      return;
  }
  push_port(addr->port());
}

void Encoder::push_headers(const std::string &command, uint32_t magic) {
  assert(size() == 0);
  push_int<uint32_t>(magic);
  append_string(command, COMMAND_SIZE);
  assert(size() == HEADER_LEN_OFFSET);

  // reserve space for len + checksum
  append_zeros(HEADER_SIZE - HEADER_LEN_OFFSET);

  assert(size() == HEADER_SIZE);
}

void Encoder::finish_headers() {
  // insert the length
  uint32_t len = htole32(size() - HEADER_SIZE);
  insert(&len, sizeof len, HEADER_LEN_OFFSET);

  // insert the checksum
  std::array<char, 4> cksum{0, 0, 0, 0};
  checksum(data() + HEADER_SIZE, size() - HEADER_SIZE, cksum);
  insert(&cksum, sizeof cksum, HEADER_CHECKSUM_OFFSET);
}

}  // namespace spv
