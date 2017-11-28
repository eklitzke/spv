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
#include <limits>

#include "./protocol.h"
#include "./reader.h"

#define CHECK(x)  \
  if (!(x)) {     \
    return false; \
  }

#define PULL(x) CHECK(pull(x))
#define PULL_SZ(x, sz) CHECK(pull(x, sz))

namespace spv {
EXTERN_LOGGER(decoder)

class Decoder : public Reader {
 public:
  Decoder() = delete;
  Decoder(const char *data, size_t sz) : Reader(data, sz) {}

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
    assert(false);  // not reached
  }

  bool pull_string(std::string &out) {
    uint64_t sz;
    CHECK(pull_varint(sz));
    assert(sz <= std::numeric_limits<uint16_t>::max());
    out.append(data(), sz);
    advance(sz);
    return true;
  }

  bool pull_headers(Headers &headers) {
    char cmd_buf[COMMAND_SIZE];
    std::memset(&cmd_buf, 0, sizeof cmd_buf);
    PULL(headers.magic);
    if (headers.magic != TESTNET3_MAGIC) {
      decoder_log->warn("peer sent wrong magic bytes");
    }

    // check that cmd_buf is null terminated before assigning
    PULL(cmd_buf);
    assert(cmd_buf[COMMAND_SIZE - 1] == '\0');
    headers.command = cmd_buf;

    PULL(headers.payload_size);
    PULL(headers.checksum);
    return true;
  }

  bool pull_netaddr(VersionNetAddr &addr) {
    PULL_SZ(addr, VERSION_NETADDR_SIZE);
    addr.port = be16toh(addr.port);  // TODO: convert ip as well
    return true;
  }

  bool pull_netaddr(NetAddr &addr) {
    PULL_SZ(addr, NETADDR_SIZE);
    addr.port = be16toh(addr.port);  // TODO: convert ip as well
    return true;
  }

  bool pull_version(Version &msg) {
    assert(offset() == HEADER_SIZE);
    PULL(msg.version);
    PULL(msg.services);
    PULL(msg.timestamp);
    CHECK(pull_netaddr(msg.addr_recv));
    if (msg.version >= 106) {
      CHECK(pull_netaddr(msg.addr_from));
      PULL(msg.nonce);
      CHECK(pull_string(msg.user_agent));
      PULL(msg.start_height);
      if (msg.version >= 70001) {
        CHECK(pull(msg.relay));
      }
    }
    return true;
  }
};
}  // namespace spv
