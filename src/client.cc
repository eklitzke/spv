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

#include <cassert>

#include "./config.h"
#include "./logging.h"
#include "./uvw.h"

namespace spv {
MODULE_LOGGER

static const std::chrono::seconds NO_REPEAT{0};

// copied from chainparams.cpp
static const std::vector<std::string> testSeeds = {
    "testnet-seed.bitcoin.jonasschnelli.ch", "seed.tbtc.petertodd.org",
    "testnet-seed.bluematt.me",
};

void Client::run() {
  log->debug("connecting to network as {}", us_.user_agent);
  for (const auto &seed : testSeeds) {
    lookup_seed(seed);
  }
}

void Client::lookup_seed(const std::string &seed) {
  auto request = loop_.resource<uvw::GetAddrInfoReq>();
  request->on<uvw::ErrorEvent>(
      [](const auto &, auto &) { log->error("dns resolution failed"); });
  request->on<uvw::AddrInfoEvent>([=](const auto &event, auto &) {
    for (const addrinfo *p = event.data.get(); p != nullptr; p = p->ai_next) {
      Addr addr(p);
      seed_peers_.insert(addr);
      log->debug("added new peer {} (via seed {})", addr, seed);
    }
    connect_to_peers();
  });
  request->nodeAddrInfo(seed);
}

void Client::connect_to_peers() {
  if (connections_.size() >= max_connections_) {
    return;
  }

  std::vector<Addr> peers(seed_peers_.cbegin(), seed_peers_.cend());
  shuffle(peers);
  while (connections_.size() < max_connections_ && !peers.empty()) {
    auto it = peers.begin();
    connect_to_peer(*it);
    if (seed_peers_.erase(*it) != 1) {
      log->error("failed to erase known peer {}", *it);
    }
    peers.erase(it);
  }
}

void Client::connect_to_peer(const Addr &addr) {
  log->debug("connecting to peer {}", addr);

  auto pr = connections_.emplace(this, addr);
  assert(pr.second);
  Connection &conn = const_cast<Connection &>(*pr.first);

  auto timer = loop_.resource<uvw::TimerHandle>();
  auto weak_timer = timer->weak_from_this();
  auto cancel_timer = [=]() {
    if (auto t = weak_timer.lock()) t->close();
  };

  conn.tcp_->once<uvw::ErrorEvent>(
      [this, cancel_timer, &conn](const auto &exc, auto &c) {
        if (exc.code() == ECONNREFUSED) {
          log->debug("peer {} refused our TCP request", conn.peer());
        } else {
          log->warn("error from peer {}: {} {} {}", conn.peer(), exc.what(),
                    exc.name(), exc.code());
        }
        cancel_timer();
        remove_connection(conn);
      });
  conn.tcp_->on<uvw::DataEvent>([&](const auto &data, auto &) {
    conn.read(data.data.get(), data.length);
  });
  conn.tcp_->once<uvw::CloseEvent>([=](const auto &, auto &) {
    log->debug("closing connection {}", addr);
    cancel_timer();
    connect_to_peers();
  });
  conn.tcp_->once<uvw::ConnectEvent>([=, &conn](const auto &, auto &) {
    log->info("connected to new peer {}", addr);
    cancel_timer();
    conn.tcp_->read();
    conn.send_version();
  });
  conn.tcp_->once<uvw::EndEvent>([=, &conn](const auto &, auto &c) {
    log->info("remote peer {} closed connection", addr);
    remove_connection(conn);
  });

  conn.connect();

  timer->once<uvw::ErrorEvent>(
      [=](const auto &, auto &timer) { timer.close(); });
  timer->on<uvw::TimerEvent>([this, &conn](const auto &, auto &timer) {
    log->warn("connection to {} timed out", conn.peer());
    remove_connection(conn);
    timer.close();
  });
  timer->start(std::chrono::seconds(1), NO_REPEAT);
}

void Client::remove_connection(Connection &conn) {
  if (conn.tcp_->closing()) {
    log->debug("ignoring remove_connection for conn already in closing state");
    return;
  }

  bool removed = connections_.erase(conn);
  if (!removed) {
    log->error("failed to remove connection to peer {}", conn.peer());
    return;
  }

  conn.tcp_->close();
}
}  // namespace spv
