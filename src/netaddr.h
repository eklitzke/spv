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
#include <cstring>

namespace spv {
struct NetAddr {
  NetAddr() : NetAddr(0) {}
  explicit NetAddr(uint16_t port)
      : time(0), services(0), port(port), sock(-1) {}
  NetAddr(const NetAddr &other) = delete;
  NetAddr(NetAddr &&other) noexcept
      : time(other.time),
        services(other.services),
        port(other.port),
        sock(other.sock) {
    memmove(&addr, &other.addr, sizeof addr);
    other.sock = -1;
  }

  // public fields
  uint32_t time;
  uint64_t services;
  char addr[16];
  uint16_t port;

  // private fields
  int sock;
};

}  // namespace spv
