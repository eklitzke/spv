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

#include "./message.h"

#include <endian.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#include "./addr.h"
#include "./buffer.h"
#include "./constants.h"
#include "./logging.h"
#include "./peer.h"
#include "./pow.h"

#define DECLARE_ENCODE(cls) \
  std::unique_ptr<char[]> cls::encode(size_t &sz) const

namespace spv {
MODULE_LOGGER

class Encoder : public Buffer {
 public:
  Encoder() = delete;
  Encoder(const Encoder &other) = delete;
  explicit Encoder(const Headers &headers) : Buffer() { push(headers); }

  template <typename T>
  void push_int(T val) {
    push(val);
  }

  void push(uint8_t val) { append(&val, sizeof val); }

  void push_zeros(size_t count) {}

  void push(uint16_t val) {
    uint16_t enc_val = htole16(val);
    append(&enc_val, sizeof enc_val);
  }

  void push(uint32_t val) {
    uint32_t enc_val = htole32(val);
    append(&enc_val, sizeof enc_val);
  }

  void push(uint64_t val) {
    uint64_t enc_val = htole64(val);
    append(&enc_val, sizeof enc_val);
  }

  // push in network byte order
  void push_be(uint16_t val) {
    uint16_t enc_val = htobe16(val);
    append(&enc_val, sizeof enc_val);
  }

  void push(CCode ccode) { push(static_cast<uint8_t>(ccode)); }

  void push(const Addr &addr) {
    addrbuf_t buf;
    addr.encode_addrbuf(buf);
    append(buf.data(), ADDR_SIZE);
    push_be(addr.port());
  }

  void push(const VersionNetAddr &addr) {
    push(addr.services);
    push(addr.addr);
  }

  void push(const NetAddr &addr) {
    push(addr.time);
    push(addr.services);
    push(addr.addr);
  }

  void push_varint(size_t val) {
    if (val < 0xfd) {
      push_int<uint8_t>(val);
      return;
    } else if (val <= 0xffff) {
      push_int<uint8_t>(0xfd);
      push_int<uint16_t>(val);
      return;
    } else if (val <= 0xffffffff) {
      push_int<uint8_t>(0xfe);
      push_int<uint32_t>(val);
      return;
    }
    push_int<uint8_t>(0xff);
    push_int<uint64_t>(val);
  }

  void push(const std::string &s) {
    push_varint(s.size());
    append(s.c_str(), s.size());
  }

  void push(const hash_t &hash) { append(hash.data(), sizeof hash); }

  void push(const Inv &inv) {
    push(inv.type);
    push(inv.hash);
  }

  void finish_headers() {
    // insert the length
    uint32_t len = htole32(size() - HEADER_SIZE);
    insert(&len, sizeof len, HEADER_LEN_OFFSET);

    // insert the checksum
    std::array<char, 4> cksum{0, 0, 0, 0};
    checksum(data() + HEADER_SIZE, size() - HEADER_SIZE, cksum);
    insert(&cksum, sizeof cksum, HEADER_CHECKSUM_OFFSET);
  }

  std::unique_ptr<char[]> serialize(size_t &sz, bool finish = true) {
    if (finish) {
      finish_headers();
    }
    return move_buffer(sz);
  }

 private:
  void push(const Headers &headers) {
    push(headers.magic);
    append_string(headers.command, COMMAND_SIZE);
    push(headers.payload_size);
    push(headers.checksum);
  }
};

DECLARE_ENCODE(AddrMsg) {
  Encoder enc(headers);
  enc.push_varint(addrs.size());
  for (const auto &addr : addrs) {
    enc.push(addr);
  }
  return enc.serialize(sz);
}

DECLARE_ENCODE(GetAddr) { return Encoder(headers).serialize(sz); }

DECLARE_ENCODE(GetBlocks) {
  Encoder enc(headers);
  enc.push(version);
  enc.push_varint(block_locators.size());
  for (const auto &locator : block_locators) {
    enc.push(locator);
  }
  enc.push(hash_stop);
  return enc.serialize(sz);
}

DECLARE_ENCODE(GetHeaders) {
  Encoder enc(headers);
  enc.push(version);
  enc.push_varint(block_locators.size());
  for (const auto &locator : block_locators) {
    enc.push(locator);
  }
  enc.push(hash_stop);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Mempool) { return Encoder(headers).serialize(sz); }

DECLARE_ENCODE(Ping) {
  Encoder enc(headers);
  enc.push(nonce);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Pong) {
  Encoder enc(headers);
  enc.push(nonce);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Reject) {
  Encoder enc(headers);
  enc.push(message);
  enc.push(ccode);
  enc.push(reason);
  if (data != empty_hash) {
    enc.push(data);
  }
  return enc.serialize(sz);
}

DECLARE_ENCODE(SendHeaders) { return Encoder(headers).serialize(sz); }

DECLARE_ENCODE(VerAck) { return Encoder(headers).serialize(sz); }

DECLARE_ENCODE(Version) {
  Encoder enc(headers);
  enc.push(version);
  enc.push(services);
  enc.push(timestamp);
  enc.push(addr_recv);
  enc.push(addr_from);
  enc.push(nonce);
  enc.push(user_agent);
  enc.push(start_height);
  enc.push(relay);
  return enc.serialize(sz);
}
}
