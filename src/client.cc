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

#include "./client.h"

#include <unistd.h>

#include <iostream>

namespace spv {

enum { PROTOCOL_VERSION = 70001 };

void Client::send_version_to_seeds(const std::vector<std::string> &seeds) {
  for (const auto &seed : seeds) {
    begin_dns_lookup(seed);
  }
}

void Client::send_version(const NetAddr &addr) {
  Encoder enc("version");
  enc.push_int<int32_t>(PROTOCOL_VERSION);    // version
  enc.push_int<uint64_t>(0);                  // services
  enc.push_time<uint64_t>();                  // timestamp
  enc.push_addr(&addr);                       // addr_recv
  enc.push_addr(nullptr);                     // addr_from
  enc.push_int<uint64_t>(connection_nonce_);  // nonce
  enc.push_string(USERAGENT);                 // user-agent
  enc.push_int<uint32_t>(1);                  // start height
  enc.push_bool(false);                       // relay

  std::cout << string_to_hex(enc.serialize()) << "\n";
}

void Client::begin_dns_lookup(const std::string &name) {
  auto request = loop_->resource<uvw::GetAddrInfoReq>();
  request->on<uvw::ErrorEvent>(
      [](const auto &, auto &) { std::cerr << "dns resolution failed\n"; });
  request->on<uvw::AddrInfoEvent>([&](uvw::AddrInfoEvent &event, auto &) {
    maybe_connect(event.data.get());
  });
  request->nodeAddrInfo(name);
}

void Client::maybe_connect(const addrinfo *info) {
  for (const addrinfo *p = info; p != nullptr; p = p->ai_next) {
    if (p->ai_family == AF_INET) {
      std::cout << "have an ipv4 address: " << p << std::endl;
    } else if (p->ai_family == AF_INET6) {
      std::cout << "have an ipv6 address: " << p << std::endl;
    }
  }
}
}  // namespace spv
