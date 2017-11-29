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

#include "./connection.h"

#include "./constants.h"
#include "./logging.h"
#include "./message.h"
#include "./uvw.h"

namespace spv {
MODULE_LOGGER

Connection::Connection(const Peer& us, Addr addr, uvw::Loop& loop)
    : us_(us), peer_(addr), tcp_(loop.resource<uvw::TcpHandle>()) {
  assert(!addr.ip().empty() && addr.port());
}

void Connection::connect() {
  log->debug("connecting to peer {}", peer_);
  uvw::Addr uvw_addr;
  uvw_addr.ip = peer_.addr.ip();
  uvw_addr.port = peer_.addr.port();
  tcp_->connect(uvw_addr);
}

void Connection::read(const char* data, size_t sz) {
  log->debug("read {} bytes from peer {}", sz, peer_);
  buf_.append(data, sz);
  for (bool ok = true; ok; ok = read_message())
    ;
}

bool Connection::read_message() {
  if (buf_.size() < HEADER_SIZE) {
    return false;
  }

  bool ret = false;
  size_t bytes_consumed = 0;
  std::unique_ptr<Message> msg =
      decode_message(buf_.data(), buf_.size(), &bytes_consumed);
  if (bytes_consumed != 0) {
    ret = true;
    buf_.consume(bytes_consumed);
  }

  if (msg.get() != nullptr) {
    const std::string& cmd = msg->headers.command;
    log->info("got message '{}' from peer {}", cmd, peer_);
    if (cmd == "addr") {
      AddrMsg* addrs = dynamic_cast<AddrMsg*>(msg.get());
      for (const auto& addr : addrs->addrs) {
        log->debug("peer sent us new addr {}", addr.addr);
      }
    } else if (cmd == "version") {
      Version* ver = dynamic_cast<Version*>(msg.get());
      peer_.nonce = ver->nonce;
      peer_.services = ver->services;
      peer_.user_agent = ver->user_agent;
      peer_.version = ver->version;
      log->info("finished handshake with peer {}", peer_);
      send_msg(VerAck{});
    } else if (cmd == "verack") {
      log->debug("ignoring verack");
    } else if (cmd == "ping") {
      Ping* ping = dynamic_cast<Ping*>(msg.get());
      Pong pong;
      pong.nonce = ping->nonce;
      send_msg(pong);
    } else if (cmd == "pong") {
      log->debug("ignoring pong");
    } else if (cmd == "sendheaders") {
      log->debug("ignoring sendheaders");
    } else {
      // not reached, decoder should have the logic to check for this
      log->error("decoder returned unknown p2p message '{}'", cmd);
    }
  }
  return ret;
}

void Connection::send_msg(const Message& msg) {
  size_t sz;
  std::unique_ptr<char[]> data = msg.encode(sz);
  const std::string& cmd = msg.headers.command;
  log->info("sending '{}' to {}", cmd, peer_);
  log->debug("message '{}' to {} will have wire size of {} bytes", cmd, peer_,
             sz);
  tcp_->write(std::move(data), sz);
}

void Connection::send_version() {
  Version ver;
  ver.version = PROTOCOL_VERSION;
  ver.services = us_.services;
  ver.addr_recv.addr = peer_.addr;
  ver.nonce = us_.nonce;
  ver.user_agent = us_.user_agent;

  send_msg(ver);
}
}  // namespace spv
