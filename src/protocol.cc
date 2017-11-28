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

#include "./encoder.h"

#define DECLARE_ENCODE(cls) \
  std::unique_ptr<char[]> cls::encode(size_t &sz) const

namespace spv {
MODULE_LOGGER

DECLARE_ENCODE(Ping) {
  Encoder enc;
  enc.push(headers);
  enc.push(nonce);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Pong) {
  Encoder enc;
  enc.push(headers);
  enc.push(nonce);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Verack) {
  Encoder enc;
  enc.push(headers);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Version) {
  Encoder enc;
  enc.push(headers);
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
