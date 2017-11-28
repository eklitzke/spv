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

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <array>
#include <cstring>

#include "./uvw.h"

namespace spv {
union inaddr {
  in_addr ipv4;
  in6_addr ipv6;
};

class Addr {
 public:
  Addr() : af_(AF_INET) {}  // TODO: this is bad
  Addr(const Addr& other) : af_(other.af_), addr_(other.addr_) {
    uvw_addr_.ip = other.uvw_addr_.ip;
    uvw_addr_.port = other.uvw_addr_.port;
  }
  explicit Addr(const addrinfo* ai);

  inline int af() const { return af_; }
  inline in_addr ipv4() const { return addr_.ipv4; }
  inline in6_addr ipv6() const { return addr_.ipv6; }
  inline uint16_t port() const { return static_cast<uint16_t>(uvw_addr_.port); }
  inline const uvw::Addr& uvw_addr() const { return uvw_addr_; }

  inline void set_addr_family(int af) { af_ = af; }
  inline void set_port(uint16_t port) {
    // caller must make sure to set this in host order
    uvw_addr_.port = port;
  }

  // must have 16 bytes of storage space
  void fill_addr_buf(std::array<char, 16>& buf) const;

  inline bool operator==(const Addr& other) const {
    return af_ == other.af_ && uvw_addr_ == other.uvw_addr_;
  }

 private:
  int af_;  // address family: AF_INET or AF_INET6
  inaddr addr_;
  uvw::Addr uvw_addr_;
};

static std::ostream& operator<<(std::ostream& o, const Addr& addr) {
  return o << addr.uvw_addr();
}
}

namespace std {
template <>
struct hash<spv::Addr> {
  std::size_t operator()(const spv::Addr& addr) const noexcept {
    std::size_t h1 = std::hash<int>{}(addr.af());
    std::size_t h2 = std::hash<uvw::Addr>{}(addr.uvw_addr());
    return h1 ^ (h2 << 1);
  }
};
}
