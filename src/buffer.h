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
#include <memory>
#include <vector>

namespace spv {
// Buffer represents a byte buffer.
class Buffer {
 public:
  Buffer() : Buffer(64) {}
  explicit Buffer(size_t cap) : capacity_(cap), size_(0), data_(new char[cap]) {
    std::memset(data_.get(), 0, cap);
  }
  Buffer(const Buffer &other) = delete;
  Buffer(Buffer &&other)
      : capacity_(other.capacity_),
        size_(other.size_),
        data_(std::move(other.data_)) {}

  // append data
  void append(const void *addr, size_t len) {
    ensure_capacity(len);
    std::memmove(data_.get() + static_cast<ptrdiff_t>(size_), addr, len);
    size_ += len;
  }

  // append string data
  void append_string(const std::string &str, size_t width = 0) {
    append(str.data(), str.size());
    if (width) {
      ssize_t zero_padding = width - str.size();
      assert(zero_padding >= 0);
      if (zero_padding) append_zeros(zero_padding);
    }
  }

  // append zeros; note that the buffer is already zero-initialized
  void append_zeros(size_t len) {
    ensure_capacity(len);
    size_ += len;
  }

  // insert directly into the middle of the buffer
  void insert(const void *addr, size_t len, size_t offset) {
    assert(offset + len <= size_);
    std::memmove(data_.get() + offset, addr, len);
  }

  inline size_t size() const { return size_; }
  inline const char *data() const { return data_.get(); }

  // this could be made more efficient
  void consume(size_t sz) {
    assert(size_ >= sz);
    std::memcpy(data_.get(), data_.get() + sz, capacity_ - sz);
    size_ -= sz;
    std::memset(data_.get() + size_, 0, capacity_ - size_);
  }

 private:
  size_t capacity_;
  size_t size_;
  std::unique_ptr<char[]> data_;

  // Ensure there's enough capacity to add len bytes.
  void ensure_capacity(size_t len);

 protected:
  std::unique_ptr<char[]> move_buffer(size_t &sz);
};
}  // namespace spv
