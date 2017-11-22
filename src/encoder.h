#pragma once

#include <endian.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <string>

#include "./picosha2.h"

#define MSG_HEADER_SIZE 24
#define COMMAND_OFFSET 4
#define PAYLOAD_OFFSET 16
#define CHECKSUM_OFFSET 20

namespace spv {
struct NetAddr {
  uint32_t time;
  uint64_t services;
  char addr[16];
  uint16_t port;

  NetAddr() { memset(this, 0, sizeof(NetAddr)); }
};

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

  void push_string(const std::string &s) {
    push_varint(s.size());
    buf_.copy(s.c_str(), s.size());
  }

  void push_varint(size_t val);
  void push_addr(const NetAddr &addr);

 private:
  void push_cstr(const char *addr, size_t len) { buf_.copy(addr, len); }
  void push_bytes(const void *addr, size_t len) { buf_.copy(addr, len); }

  Buffer buf_;
};

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

void Encoder::push_addr(const NetAddr &addr) {
  push_int<uint32_t>(addr.time);
  push_int<uint64_t>(addr.services);
  push_bytes(reinterpret_cast<const void *>(&addr.addr), sizeof addr.addr);
  push_int<uint16_t>(addr.port);
}

}  // namespace spv
