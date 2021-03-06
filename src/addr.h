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

namespace spv {
typedef std::array<uint8_t, 16> addrbuf_t;

union inaddr {
  in_addr ipv4;
  in6_addr ipv6;

  inaddr() { std::memset(this, 0, sizeof(inaddr)); }
};

class Addr {
 public:
  Addr() : af_(-1), port_(0) {}
  Addr(const Addr& other)
      : af_(other.af_),
        port_(other.port_),
        inaddr_(other.inaddr_),
        ip_(other.ip_) {}
  explicit Addr(const addrinfo* ai);

  inline int af() const { return af_; }
  inline uint16_t port() const { return port_; }
  inline const std::string& ip() const { return ip_; }

  inline void set_port(uint16_t port) { port_ = port; }  // in host order
  void set_addr(const addrbuf_t& buf);

  void encode_addrbuf(addrbuf_t& buf) const;

  inline bool operator==(const Addr& other) const {
    return af_ == other.af_ && port_ == other.port_ && ip_ == other.ip_;
  }
  inline bool operator!=(const Addr& other) const { return !operator==(other); }

 private:
  int af_;  // address family: AF_INET or AF_INET6
  uint16_t port_;
  inaddr inaddr_;
  std::string ip_;
};
}  // namespace spv

std::ostream& operator<<(std::ostream& o, const spv::Addr& addr);

namespace std {
template <>
struct hash<spv::Addr> {
  std::size_t operator()(const spv::Addr& addr) const noexcept {
    std::size_t h1 = std::hash<uint16_t>{}(addr.port());
    std::size_t h2 = std::hash<std::string>{}(addr.ip());
    return h1 ^ (h2 << 1);
  }
};
}  // namespace std
