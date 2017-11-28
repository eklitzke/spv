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

#include <memory>
#include <utility>

#include "./addr.h"
#include "./buffer.h"
#include "./decoder.h"
#include "./uvw.h"

namespace spv {
class Connection {
 public:
  Connection() = delete;
  Connection(Addr addr, uvw::Loop& loop)
      : addr_(addr),
        tcp_(loop.resource<uvw::TcpHandle>()),
        our_nonce_(0),
        their_nonce_(0),
        our_services_(0),
        their_services_(0) {}
  Connection(const Connection& other) = delete;

  const Addr& addr() const { return addr_; }

  inline uvw::TcpHandle* tcp() { return tcp_.get(); }
  inline uvw::TcpHandle* operator->() { return tcp_.get(); }

  inline bool operator==(const Connection& other) const {
    return this == &other;
  }

  // establish the connection
  void connect();

  // read data
  void read(const char* data, size_t sz);

  void send_version(uint64_t nonce, uint64_t services);

 private:
  Addr addr_;
  Buffer read_buf_;
  std::shared_ptr<uvw::TcpHandle> tcp_;

  uint64_t our_nonce_, their_nonce_;
  uint64_t our_services_, their_services_;

  inline void consume(const Decoder& dec) {
    read_buf_.consume(dec.bytes_read());
  }
};
}  // namespace spv

namespace std {
template <>
struct hash<spv::Connection> {
  std::size_t operator()(const spv::Connection& conn) const noexcept {
    return std::hash<uvw::Addr>{}(conn.addr().uvw_addr());
  }
};
}
