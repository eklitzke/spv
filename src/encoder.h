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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <string>

#include "./addr.h"
#include "./buffer.h"

namespace spv {
// Encoder can encode data.
class Encoder {
 public:
  explicit Encoder(const std::string &command) {
    if (!command.empty()) {
      buf_.allocate_headers(command);
    }
  }

  inline std::string serialize(bool finish = true) {
    if (finish) buf_.finish_headers();
    return buf_.string();
  }

  template <typename T>
  void push_int(T val) {
    buf_.copy(&val, sizeof val);
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
    buf_.copy(s.c_str(), s.size());
  }

  void push_varint(size_t val);

  void push_netaddr(const Addr &addr, uint64_t services = 0,
                    bool include_time = false);

 private:
  void push_bytes(const void *addr, size_t len) { buf_.copy(addr, len); }

  Buffer buf_;
};
}  // namespace spv
