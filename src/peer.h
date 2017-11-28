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

#include <cstdint>
#include <ostream>
#include <string>

#include "./addr.h"

namespace spv {
struct Peer {
  uint32_t nonce;
  uint32_t services;
  uint32_t version;
  std::string user_agent;
  Addr addr;

  Peer() : nonce(0), services(0), version(0) {}
  explicit Peer(const Addr& addr) : addr(addr) {}
  Peer(uint32_t n, uint32_t s, uint32_t v, const std::string& ua)
      : nonce(n), services(s), version(v), user_agent(ua) {}
  Peer(const Peer& other)
      : nonce(other.nonce),
        services(other.services),
        version(other.version),
        user_agent(other.user_agent),
        addr(other.addr) {}
};
}  // namespace spv

std::ostream& operator<<(std::ostream& o, const spv::Peer& p);
