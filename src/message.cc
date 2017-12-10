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
#include "./encoder.h"
#include "./logging.h"
#include "./peer.h"
#include "./pow.h"

#define DECLARE_ENCODE(cls) \
  std::unique_ptr<char[]> cls::encode(size_t &sz) const

namespace spv {
MODULE_LOGGER

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
  enc.push_varint(locator_hashes.size());
  for (const auto &locator : locator_hashes) {
    enc.push(locator);
  }
  enc.push(hash_stop);
  return enc.serialize(sz);
}

DECLARE_ENCODE(GetHeaders) {
  Encoder enc(headers);
  enc.push(version);
  enc.push_varint(locator_hashes.size());
  for (const auto &locator : locator_hashes) {
    enc.push(locator);
  }
  enc.push(hash_stop);
  return enc.serialize(sz);
}

DECLARE_ENCODE(HeadersMsg) {
  Encoder enc(headers);
  enc.push_varint(block_headers.size());
  for (const auto &hdr : block_headers) {
    enc.push(hdr);
  }
  return enc.serialize(sz);
}

DECLARE_ENCODE(Inv) {
  Encoder enc(headers);
  enc.push(type);
  enc.push(hash);
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
