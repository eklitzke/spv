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

#include <endian.h>
#include <netinet/in.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#include "./addr.h"
#include "./buffer.h"
#include "./protocol.h"

namespace spv {
enum { IPV4_PADDING = sizeof(in6_addr) - sizeof(in_addr) };
static_assert(sizeof(in_addr) == 4);
static_assert(sizeof(in6_addr) == 16);
static_assert(IPV4_PADDING == 12);

// Encoder can encode data.
class Encoder {
 public:
  Encoder() {}
  explicit Encoder(const std::string &command) { set_command(command); }

  template <typename T>
  void push_int(T val) {
    buf_.append(&val, sizeof val);
  }

  inline void push_byte(uint8_t val) { return push_int<uint8_t>(val); }

  template <typename T>
  void push_time() {
    time_t tv = time(nullptr);
    push_int<T>(static_cast<T>(tv));
  }

  inline void push_bool(bool val) { return push_byte(val); }

  inline void push_string(const std::string &s) {
    push_varint(s.size());
    buf_.append(s.c_str(), s.size());
  }

  void push_varint(size_t val);

  // addr may be null
  void push_netaddr(const Addr *addr, uint64_t services = 0,
                    bool include_time = false);

  inline void push_netaddr(const Addr &addr, uint64_t services = 0,
                           bool include_time = false) {
    push_netaddr(&addr, services, include_time);
  }

  // allocate message headers, reserving the required space
  void set_command(const std::string &command, uint32_t magic = TESTNET3_MAGIC);

  std::unique_ptr<char[]> move_buffer(size_t *sz, bool finish = true) {
    if (finish) finish_headers();
    return buf_.move_buffer(sz);
  }

 private:
  void push_bytes(const void *addr, size_t len) { buf_.append(addr, len); }
  void finish_headers();

  inline void push_ipv4(const in_addr &addr) {
    buf_.append_zeros(IPV4_PADDING);
    buf_.append(&addr.s_addr, sizeof addr);
  }

  inline void push_ipv6(const in6_addr &addr) {
    buf_.append(&addr.s6_addr, sizeof addr);
  }

  inline void push_port(uint16_t port) {
    uint16_t be_port = htobe16(port);
    buf_.append(&be_port, sizeof be_port);
  }

  Buffer buf_;
};
}  // namespace spv
