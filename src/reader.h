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
#include <memory>
#include <type_traits>

#include "./logging.h"

namespace spv {
EXTERN_LOGGER(reader)

// Reader reads through a buffer
class Reader {
 public:
  Reader(const char* data, size_t cap) : data_(data), cap_(cap), off_(0) {}

  inline bool pull(char* out, size_t sz) {
    if (sz + off_ > cap_) {
      reader_log->warn("failed to pull {} bytes", sz);
      return false;
    }
    memmove(out, data_ + off_, sz);
    off_ += sz;
    return true;
  }

  template <typename T>
  inline bool pull(T& out) {
    return pull(reinterpret_cast<char*>(&out), sizeof(T));
  }

  // the "size" of bytes read, which internally we call the offset
  inline size_t size() const { return off_; }

 private:
  const char* data_;
  size_t cap_;
  size_t off_;
};
}  // namespace spv
