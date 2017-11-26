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

#include <sys/socket.h>
#include <unistd.h>

#include "./logging.h"
#include "./util.h"

namespace spv {

static const uvw::TimerHandle::Time NO_REPEAT(0);
static const uvw::TimerHandle::Time CONNECT_TIMEOUT(1000);

DEFINE_LOGGER

enum { PROTOCOL_VERSION = 70001 };

// testnet port
enum { DEFAULT_PORT = 18332 };

void Client::send_version_to_seeds(const std::vector<std::string> &seeds) {
  for (const auto &seed : seeds) {
    lookup_seed(seed);
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

  log->info("version message is: {}", string_to_hex(enc.serialize()));
}

void Client::lookup_seed(const std::string &seed) {
  auto request = loop_->resource<uvw::GetAddrInfoReq>();
  request->on<uvw::ErrorEvent>(
      [](const auto &, auto &) { log->error("dns resolution failed"); });
  request->on<uvw::AddrInfoEvent>([=](uvw::AddrInfoEvent &event, auto &) {
    char buf[INET6_ADDRSTRLEN];
    memset(buf, 0, sizeof buf);

    sockaddr_in *sa4;
    sockaddr_in6 *sa6;
    uvw::Addr addr;
    addr.port = DEFAULT_PORT;

    for (const addrinfo *p = event.data.get(); p != nullptr; p = p->ai_next) {
      switch (p->ai_addr->sa_family) {
        case AF_INET:
          sa4 = reinterpret_cast<sockaddr_in *>(p->ai_addr);
          if (inet_ntop(sa4->sin_family, &sa4->sin_addr, buf, sizeof buf) ==
              nullptr) {
            log->error("inet_ntop failed: {}", strerror(errno));
            continue;
          }
          break;
        case AF_INET6:
          sa6 = reinterpret_cast<sockaddr_in6 *>(p->ai_addr);
          if (inet_ntop(sa6->sin6_family, &sa6->sin6_addr, buf, sizeof buf) ==
              nullptr) {
            log->error("inet_ntop failed: {}", strerror(errno));
            continue;
          }
          break;
        default:
          log->warn("unknown address family {}", p->ai_addr->sa_family);
          break;
      }
      addr.ip = buf;
      auto x = known_peers_.insert(addr);
      if (x.second) {
        log->debug("added peer {} (from seed {})", addr, seed);
      }
    }
    connect_to_peers();
  });
  request->nodeAddrInfo(seed);
}

void Client::connect_to_peers() {
  if (connections_.size() >= max_connections_) {
    return;
  }

  std::vector<uvw::Addr> addrs(known_peers_.cbegin(), known_peers_.cend());
  shuffle(addrs);
  while (connections_.size() < max_connections_ && !addrs.empty()) {
    auto peer = addrs.begin();
    connect_to_peer(*peer);
    addrs.erase(peer);
    if (known_peers_.erase(*peer) != 1) {
      log->error("failed to erase known peer {}", *peer);
    }
  }
}

void Client::connect_to_peer(const uvw::Addr &addr) {
  auto conn = std::make_shared<Connection>(addr);
  conn->tcp->on<uvw::ErrorEvent>(
      [=](const uvw::ErrorEvent &, uvw::TcpHandle &) {
        log->error("got a tcp error from {}", conn->addr);
        remove_connection(conn.get());
        connect_to_peers();
        conn->reset();
      });
  conn->tcp->once<uvw::ConnectEvent>([=](const auto &, auto &) {
    log->info("!!!! connected to {}", conn->addr);
    remove_connection(conn.get());
    conn->reset();
  });
  conn->timer->on<uvw::TimerEvent>([=](const auto &, auto &) {
    log->warn("connection to {} timed out", conn->addr);
    remove_connection(conn.get());
    connect_to_peers();
  });

  log->info("connecting to {}", conn->addr);
  conn->tcp->connect(addr);
  conn->timer->start(CONNECT_TIMEOUT, NO_REPEAT);
  connections_.push_back(conn);
}

void Client::remove_connection(const Connection *conn) {
  bool removed = false;
  for (auto it = connections_.begin(); it != connections_.end(); it++) {
    if ((*it)->addr == conn->addr) {
      connections_.erase(it);
      removed = true;
      break;
    }
  }
  if (!removed) {
    log->error("failed to remove connection {}", conn->addr);
  }
}
}  // namespace spv
