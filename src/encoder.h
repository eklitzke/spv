#pragma once

#include <endian.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <string>

#include "./picosha2.h"

#define MSG_HEADER_SIZE 24
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
    assert(data_ != nullptr);
  }
  ~Buffer() { free(data_); }

  // allocate message headers, reserving the required space
  void allocate_headers(const std::string &cmd) {
    assert(size_ == 0);
    assert(cmd.size() <= 12);
    ensure_capacity(MSG_HEADER_SIZE);

    static const uint32_t testnet_magic = 0x0b110907;
    copy(&testnet_magic, sizeof testnet_magic);

    char cmd_bytes[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    memmove(&cmd_bytes, cmd.c_str(), cmd.size());

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
    memmove(hash2.data(), data_ + CHECKSUM_OFFSET, 4);
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
    size_t orig_capacity = capacity_;
    while (size_ + len > capacity_) {
      capacity_ *= 2;
    }
    if (capacity_ > orig_capacity) {
      // Realloc the buffer, and set the extra bytes to 0 for good hygiene.
      data_ = reinterpret_cast<unsigned char *>(realloc(data_, capacity_));
      memset(data_ + size_, 0, capacity_ - orig_capacity);
    }
  }
};

// Encoder can encode data.
class Encoder {
 public:
  explicit Encoder(const std::string &command) {
    if (!command.empty()) {
      buf_.allocate_headers(command);
    }
  }

  inline std::string serialize() { return buf_.string(); }

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
