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

#include <array>
#include <limits>

#include "./buffer.h"
#include "./logging.h"
#include "./pow.h"
#include "./protocol.h"

#define CHECK(x)                                            \
  if (!(x)) {                                               \
    decoder_log->warn("CHECK failed on line {}", __LINE__); \
    return false;                                           \
  }

#define PULL(x) CHECK(pull(x))

namespace spv {
EXTERN_LOGGER(decoder)

class Decoder {
 public:
  Decoder() = delete;
  Decoder(const char *data, size_t sz) : data_(data), cap_(sz), off_(0) {}

  bool replace(size_t cap) {
    if (cap <= cap_ && cap <= off_) {
      cap_ = cap;
      return true;
    }
    return false;
  }

  bool pull(uint8_t &out) { return pull(&out, sizeof out); }

  bool pull(uint16_t &out) {
    bool ok = pull(&out, sizeof out);
    out = le16toh(out);
    return ok;
  }

  bool pull(uint32_t &out) {
    bool ok = pull(&out, sizeof out);
    out = le32toh(out);
    return ok;
  }

  bool pull(uint64_t &out) {
    bool ok = pull(&out, sizeof out);
    out = le64toh(out);
    return ok;
  }

  bool pull_be(uint16_t &out) {
    bool ok = pull(&out, sizeof out);
    out = be16toh(out);
    return ok;
  }

  bool pull(std::string &out) {
    uint64_t sz;
    CHECK(pull_varint(sz));
    CHECK(sz <= std::numeric_limits<uint16_t>::max());
    out.append(data(), sz);
    off_ += sz;
    return true;
  }

  bool pull(Headers &headers) {
    std::array<char, COMMAND_SIZE> cmd_buf{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    PULL(headers.magic);
    if (headers.magic != TESTNET3_MAGIC) {
      decoder_log->warn("peer sent wrong magic bytes");
    }
    CHECK(pull(cmd_buf.data(), COMMAND_SIZE));
    CHECK(cmd_buf[COMMAND_SIZE - 1] == '\0');
    headers.command = cmd_buf.data();
    PULL(headers.payload_size);
    PULL(headers.checksum);
    return true;
  }

  bool pull(Addr &addr) {
    // TODO: need to actually use addr_buf
    std::array<char, ADDR_SIZE> addr_buf{0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0};
    CHECK(pull(addr_buf.data(), ADDR_SIZE));

    // port is in network byte order
    uint16_t port;
    CHECK(pull_be(port));
    addr.set_port(port);
    return true;
  }

  bool pull(VersionNetAddr &addr) {
    PULL(addr.services);
    PULL(addr.addr);
    return true;
  }

  bool pull(NetAddr &addr) {
    PULL(addr.time);
    PULL(addr.services);
    PULL(addr.addr);
    return true;
  }

  bool pull(Ping &msg) {
    PULL(msg.nonce);
    return true;
  }

  bool pull(Pong &msg) {
    PULL(msg.nonce);
    return true;
  }

  bool pull(Version &msg) {
    PULL(msg.version);
    PULL(msg.services);
    PULL(msg.timestamp);
    PULL(msg.addr_recv);
    if (msg.version >= 106) {
      PULL(msg.addr_from);
      PULL(msg.nonce);
      PULL(msg.user_agent);
      PULL(msg.start_height);
      if (msg.version >= 70001) {
        PULL(msg.relay);
      }
    }
    return true;
  }

  bool pull(Verack &msg) { return true; }

  bool validate_msg(const Message &msg) {
    if (off_ != cap_) {
      decoder_log->warn("failed to pull enough bytes");
      return false;
    }
    uint32_t cksum = checksum(data_, cap_);
    if (cksum != msg.headers.checksum) {
      decoder_log->warn("invalid checksum!");
      return false;
    }
    return true;
  }

  inline size_t bytes_read() const { return off_; }
  inline const char *data() const { return data_ + off_; }

 private:
  const char *data_;
  size_t cap_;
  size_t off_;

  bool pull(void *out, size_t sz) {
    if (sz + off_ > cap_) {
      decoder_log->debug("failed to pull {} bytes, offset = {}, capacity = {}",
                         sz, off_, cap_);
      return false;
    }
    std::memmove(out, data_ + off_, sz);
    off_ += sz;
    return true;
  }

  bool pull_varint(uint64_t &out) {
    uint8_t prefix;
    PULL(prefix);
    if (prefix < 0xfd) {
      out = prefix;
      return true;
    }
    switch (prefix) {
      case 0xfd: {
        uint16_t val;
        PULL(val);
        out = val;
        return true;
      }
      case 0xfe: {
        uint32_t val;
        PULL(val);
        out = val;
        return true;
      }
      case 0xff:
        PULL(out);
        return true;
    }
    return false;  // not reached
  }
};
}  // namespace spv
