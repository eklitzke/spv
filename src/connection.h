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
#include "./config.h"
#include "./message.h"
#include "./peer.h"
#include "./util.h"

namespace uvw {
class TcpHandle;
class TimerHandle;
class Loop;
class Addr;
}  // namespace uvw

namespace spv {
class Connection {
 public:
  Connection() = delete;
  Connection(const Peer& us, Addr them, uvw::Loop& loop);
  Connection(const Connection& other) = delete;

  const Peer& peer() const { return peer_; }

  inline uvw::TcpHandle* tcp() { return tcp_.get(); }
  inline uvw::TcpHandle* operator->() { return tcp_.get(); }

  inline bool operator==(const Connection& other) const {
    return this == &other;
  }

  // establish the connection
  void connect();

  // read data
  void read(const char* data, size_t sz);

  void send_version();

 private:
  Buffer buf_;
  const Peer& us_;
  Peer peer_;
  uvw::Loop& loop_;
  std::shared_ptr<uvw::TcpHandle> tcp_;

  // heartbeat information
  uint64_t ping_nonce_;
  std::shared_ptr<uvw::TimerHandle> ping_;
  std::shared_ptr<uvw::TimerHandle> pong_;

  // returns true if a message was actually read
  bool read_message();

  // send a message to our peer
  void send_msg(const Message& msg);

  // close this connection (e.g. because we have a bad peer)
  void shutdown();

  void handle_addr(AddrMsg* addrs);
  void handle_getaddr(GetAddr* getaddr);
  void handle_getblocks(GetBlocks* getblocks);
  void handle_getheaders(GetHeaders* getheaders);
  void handle_mempool(Mempool* pool);
  void handle_ping(Ping* ping);
  void handle_pong(Pong* pong);
  void handle_reject(Reject* rej);
  void handle_sendheaders(SendHeaders* send);
  void handle_unknown(const std::string& msg);
  void handle_verack(VerAck* ack);
  void handle_version(Version* ver);
};
}  // namespace spv

namespace std {
template <>
struct hash<spv::Connection> {
  std::size_t operator()(const spv::Connection& conn) const noexcept {
    return std::hash<spv::Addr>{}(conn.peer().addr);
  }
};
}  // namespace std
