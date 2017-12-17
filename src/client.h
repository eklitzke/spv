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

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "./addr.h"
#include "./buffer.h"
#include "./chain.h"
#include "./config.h"
#include "./connection.h"
#include "./peer.h"
#include "./settings.h"
#include "./util.h"

namespace uvw {
class Loop;
class GetAddrInfoReq;
}

namespace spv {
class Addr;

class Client {
  friend Connection;

 public:
  Client(const Settings &settings, std::shared_ptr<uvw::Loop> loop);
  Client() = delete;
  Client(const Client &other) = delete;

  // Send the version message to these seeds.
  void run();

  void shutdown();

 private:
  const Settings &settings_;
  std::unordered_set<Addr> seed_peers_;
  std::unordered_set<NetAddr> peers_;
  std::unordered_map<Addr, std::unique_ptr<Connection> > connections_;
  std::unordered_set<Inv> pending_inv_;
  Buffer read_buf_;
  bool shutdown_;
  bool need_headers_;
  Chain chain_;

  std::vector<std::shared_ptr<uvw::GetAddrInfoReq> > dns_requests_;

  std::shared_ptr<uvw::TimerHandle> hdr_timeout_;

  // cancel the hdr timeout
  void cancel_hdr_timeout();

  // cancel all outstanding dns requests
  void cancel_dns_requests();

  // mark this request as completed
  void remove_dns_request(uvw::GetAddrInfoReq *req);

 protected:
  Peer us_;
  std::shared_ptr<uvw::Loop> loop_;

  // Connections call this method to notify the client that they've finished
  // connecting (meaning they've finished the version handshake). Then the
  // client will ask the connections for more block headers.
  void notify_connected(Connection *conn);

  // Add headers to the local copy of the chain.
  void notify_headers(const std::vector<BlockHeader> &block_headers);

  // Connections call this method to notify the client of a new peer.
  void notify_peer(const NetAddr &addr);

  // notify that there was an error
  void notify_error(Connection *conn, const std::string &why);

  // notify of a new inv message
  void notify_inv(Connection *conn, const Inv &inv);

  // find a new addr and connect to it
  void connect_to_new_peer();

 private:
  // get peers from a dns seed
  void lookup_seed(const std::string &seed);

  // connect to a specific address
  void connect_to_addr(const Addr &addr);
  void connect_to_addr(const NetAddr &addr) {
    return connect_to_addr(addr.addr);
  }

  // enqueue connections
  void remove_connection(Connection *conn);

  // select a random connection
  Connection *random_connection();

  // try to get more headers
  void sync_more_headers(Connection *conn = nullptr);

  Addr select_peer() const;

  // are we connected to this addr?
  bool is_connected_to_addr(const Addr &addr) const;
  bool is_connected_to_addr(const NetAddr &addr) const {
    return is_connected_to_addr(addr.addr);
  }
};
}  // namespace spv
