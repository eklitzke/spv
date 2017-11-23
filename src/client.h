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
#include <vector>

#include "./config.h"
#include "./encoder.h"
#include "./util.h"

#include "../third_party/uvw/src/uvw.hpp"

namespace spv {
// copied from chainparams.cpp
static const std::vector<std::string> testSeeds = {
    "testnet-seed.bitcoin.jonasschnelli.ch", "seed.tbtc.petertodd.org",
    "testnet-seed.bluematt.me",
};

// testnet port
enum { DEFAULT_PORT = 18333 };

class Client {
 public:
  Client() : connection_nonce_(rand64()), loop_(uvw::Loop::getDefault()) {}
  Client(const Client &other) = delete;

  // Send the version message to these seeds.
  void send_version_to_seeds(const std::vector<std::string> &seeds = testSeeds);

 private:
  const uint64_t connection_nonce_;
  std::shared_ptr<uvw::Loop> loop_;

  // start a DNS lookup
  void begin_dns_lookup(const std::string &name);

  // connect to a DNS name
  void maybe_connect(const addrinfo *info);

  // Send a version message.
  void send_version(const NetAddr &addr);
};
}  // namespace spv
