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

#include "./decoder.h"
#include "./encoder.h"
#include "./logging.h"
#include "./message.h"

namespace spv {
MODULE_LOGGER

void Connection::connect() {
  log->debug("connecting to peer {}", peer_);
  tcp_->connect(peer_.addr.uvw_addr());
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
      decode_message(peer_, buf_.data(), buf_.size(), &bytes_consumed);
  if (bytes_consumed != 0) {
    ret = true;
    buf_.consume(bytes_consumed);
  }

  if (msg.get() != nullptr) {
    const std::string& cmd = msg->headers.command;
    log->info("got message '{}' from peer {}", cmd, peer_);
    if (cmd == "version") {
      Version* ver = dynamic_cast<Version*>(msg.get());
      peer_.nonce = ver->nonce;
      peer_.services = ver->services;
      peer_.user_agent = ver->user_agent;
      peer_.version = ver->version;
      log->info("finished handshake with peer {}", peer_);
      send_msg(Verack{});
    } else if (cmd == "verack") {
      // no-op
    } else if (cmd == "ping") {
      Ping* ping = dynamic_cast<Ping*>(msg.get());
      Pong pong;
      pong.nonce = ping->nonce;
      send_msg(pong);
    } else if (cmd == "pong") {
      // no-op
    } else {
      // not reached, decoder.cc already check this
      log->warn("unhandle p2p message type '{}'", cmd);
    }
  }
  return ret;
}

void Connection::send_msg(const Message& msg) {
  size_t sz;
  std::unique_ptr<char[]> data = msg.encode(sz);
  log->info("sending '{}' to {}, size={}", msg.headers.command, peer_, sz);
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
