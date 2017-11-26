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

#include "./config.h"
#include "./encoder.h"
#include "./logging.h"
#include "./util.h"
#include "./uvw.h"

#define MAX_CONNECTIONS 2

namespace spv {
// copied from chainparams.cpp
static const std::vector<std::string> testSeeds = {
    "testnet-seed.bitcoin.jonasschnelli.ch", "seed.tbtc.petertodd.org",
    "testnet-seed.bluematt.me",
};

struct Connection {
  Connection() = delete;
  explicit Connection(const uvw::Addr &addr_) {
    auto loop = uvw::Loop::getDefault();
    tcp = loop->resource<uvw::TcpHandle>();
    timer = loop->resource<uvw::TimerHandle>();
    addr.ip = addr_.ip;
    addr.port = addr_.port;
  }

  inline void reset() {
    tcp->close();
    tcp = nullptr;
    timer->stop();
    timer = nullptr;
  }

  uvw::Addr addr;
  std::shared_ptr<uvw::TcpHandle> tcp;
  std::shared_ptr<uvw::TimerHandle> timer;
};

class Client {
 public:
  Client()
      : max_connections_(MAX_CONNECTIONS),
        connection_nonce_(rand64()),
        loop_(uvw::Loop::getDefault()) {}
  Client(const Client &other) = delete;

  // Send the version message to these seeds.
  void send_version_to_seeds(const std::vector<std::string> &seeds = testSeeds);

 private:
  const size_t max_connections_;
  const uint64_t connection_nonce_;
  std::shared_ptr<uvw::Loop> loop_;
  std::unordered_set<uvw::Addr> known_peers_;
  std::vector<std::shared_ptr<Connection>> connections_;

  // get peers from a dns seed
  void lookup_seed(const std::string &seed);

  // try to keep connections full
  void connect_to_peers();

  // connect to a specific peer
  void connect_to_peer(const uvw::Addr &addr);

  // enqueue connections
  void remove_connection(const Connection *conn);

  // Send a version message.
  void send_version(const NetAddr &addr);
};
}  // namespace spv
