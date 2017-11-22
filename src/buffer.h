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

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "../third_party/PicoSHA2/picosha2.h"

#define MSG_HEADER_SIZE 24
#define COMMAND_OFFSET 4
#define PAYLOAD_OFFSET 16
#define CHECKSUM_OFFSET 20

namespace spv {
// Buffer represents a byte buffer.
class Buffer {
 public:
  Buffer() : Buffer(64) {}
  explicit Buffer(size_t cap)
      : capacity_(cap),
        size_(0),
        data_(reinterpret_cast<unsigned char *>(malloc(cap))) {
    if (data_ == nullptr) {
      throw std::bad_alloc();
    }
  }
  ~Buffer() { free(data_); }

  // allocate message headers, reserving the required space
  void allocate_headers(const std::string &cmd) {
    assert(size_ == 0);
    assert(cmd.size() <= 12);
    ensure_capacity(MSG_HEADER_SIZE);

    static const uint32_t testnet_magic = 0x0b110907;
    copy(&testnet_magic, sizeof testnet_magic);

    memmove(data_ + COMMAND_OFFSET, cmd.c_str(), cmd.size());

    size_ = 24;
  }

  void finish_headers() {
    // insert the length
    uint32_t len = htole32(size_ - MSG_HEADER_SIZE);
    memmove(data_ + PAYLOAD_OFFSET, &len, sizeof len);

    // insert the checksum
    std::vector<unsigned char> hash1(32), hash2(32);
    picosha2::hash256(data_ + MSG_HEADER_SIZE, data_ + size_, hash1);
    picosha2::hash256(hash1, hash2);
    memmove(data_ + CHECKSUM_OFFSET, hash2.data(), 4);
  }

  void copy(const void *addr, size_t len) {
    ensure_capacity(len);
    memmove(data_ + static_cast<ptrdiff_t>(size_), addr, len);
    size_ += len;
  }

  inline size_t size() const { return size_; }
  inline unsigned char *data() const { return data_; }
  inline std::string string() const {
    return std::string(reinterpret_cast<const char *>(data_), size_);
  }

 private:
  size_t capacity_;
  size_t size_;
  unsigned char *data_;

  // Ensure there's enough capacity to add len bytes.
  void ensure_capacity(size_t len) {
    size_t new_capacity = capacity_;
    while (size_ + len > new_capacity) {
      new_capacity *= 2;
    }
    if (new_capacity > capacity_) {
      // Realloc the buffer, and set the extra bytes to 0 for good hygiene.
      void *p = realloc(data_, new_capacity);
      if (p == nullptr) {
        throw std::bad_alloc();
      }
      data_ = reinterpret_cast<unsigned char *>(p);
      memset(data_ + capacity_, 0, new_capacity - capacity_);
      capacity_ = new_capacity;
    }
  }
};

}  // namespace spv
