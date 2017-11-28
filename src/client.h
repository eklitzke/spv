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
#include <string>
#include <unordered_set>
#include <vector>

#include "./addr.h"
#include "./buffer.h"
#include "./connection.h"
#include "./encoder.h"
#include "./logging.h"
#include "./util.h"
#include "./uvw.h"

namespace spv {
class Client {
 public:
  Client(uvw::Loop &loop, size_t max_connections)
      : max_connections_(max_connections),
        nonce_(rand64()),
        services_(0),
        loop_(loop) {}

  Client() = delete;
  Client(const Client &other) = delete;

  // Send the version message to these seeds.
  void run();

 private:
  const size_t max_connections_;
  const uint64_t nonce_;
  const uint64_t services_;
  uvw::Loop &loop_;
  std::unordered_set<Addr> known_peers_;
  std::unordered_set<Connection> connections_;
  Buffer read_buf_;

  // get peers from a dns seed
  void lookup_seed(const std::string &seed);

  // try to keep connections full
  void connect_to_peers();

  // connect to a specific peer
  void connect_to_peer(const Addr &addr);

  // enqueue connections
  void remove_connection(Connection &conn, bool reconnect = true);
};
}  // namespace spv
