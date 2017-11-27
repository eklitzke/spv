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
#include "./uvw.h"

namespace spv {
class Connection {
 public:
  Connection() = delete;
  Connection(Addr addr, uvw::Loop& loop)
      : addr_(addr), tcp_(loop.resource<uvw::TcpHandle>()) {}
  Connection(Connection&& other)
      : addr_(other.addr_),
        read_buf_(std::move(other.read_buf_)),
        tcp_(std::move(other.tcp_)) {}
  Connection(const Connection& other) = delete;

  uvw::TcpHandle* operator->() { return tcp_.get(); }

  void connect() { tcp_->connect(addr_.uvw_addr()); }
  const Addr& addr() const { return addr_; }

  inline bool operator==(const Connection& other) const {
    return this == &other;
  }

 private:
  Addr addr_;
  Buffer read_buf_;
  std::shared_ptr<uvw::TcpHandle> tcp_;
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
