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
#include "./config.h"
#include "./connection.h"
#include "./peer.h"
#include "./util.h"

namespace uvw {
class Loop;
}

namespace spv {
class Addr;
class Connection;

class Client {
  friend Connection;

 public:
  Client(uvw::Loop &loop, size_t max_connections)
      : max_connections_(max_connections),
        us_(rand64(), 0, PROTOCOL_VERSION, USER_AGENT),
        loop_(loop) {}

  Client() = delete;
  Client(const Client &other) = delete;

  // Send the version message to these seeds.
  void run();

 private:
  size_t max_connections_;
  std::unordered_set<Addr> known_peers_;
  std::unordered_set<Connection> connections_;
  Buffer read_buf_;

 protected:
  Peer us_;
  uvw::Loop &loop_;

 private:
  // get peers from a dns seed
  void lookup_seed(const std::string &seed);

  // try to keep connections full
  void connect_to_peers();

  // connect to a specific peer
  void connect_to_peer(const Addr &addr);

  // enqueue connections
  void remove_connection(Connection &conn);
};
}  // namespace spv
