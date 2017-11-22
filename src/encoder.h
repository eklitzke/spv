#pragma once

#include <endian.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace spv {

// Buffer represents a byte buffer.
class Buffer {
 public:
  void copy(void *addr, size_t len) {
    ensure_capacity(len);
    memmove(data_ + static_cast<ptrdiff_t>(length_), addr, len);
    length_ += len;
  }

 private:
  void *data_;
  size_t capacity_;
  size_t length_;

  // Ensure there's enough capacity to add len bytes.
  void ensure_capacity(size_t len) {
    while (length_ + len > capacity_) {
      capacity_ *= 2;
    }
    data_ = realloc(data_, capacity_);
  }
};

// Encoder can encode data.
class Encoder {
 public:
  template <typename T>
  void push(T val) {
    buf_.copy(&val, sizeof val);
  }

 private:
  void push_bytes(void *addr, size_t len) { buf_.copy(addr, len); }

  Buffer buf_;
};

template <>
void Encoder::push(uint16_t val) {
  val = htole16(val);
  buf_.copy(&val, sizeof val);
}

template <>
void Encoder::push(uint32_t val) {
  val = htole32(val);
  buf_.copy(&val, sizeof val);
}

template <>
void Encoder::push(uint64_t val) {
  val = htole64(val);
  buf_.copy(&val, sizeof val);
}

}  // namespace spv
