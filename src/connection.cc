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

#include "./config.h"
#include "./decoder.h"
#include "./encoder.h"
#include "./logging.h"
#include "./protocol.h"

#define PULL_MSG(cmd)                                 \
  cmd.headers = hdrs;                                 \
  if (dec.pull(cmd)) {                                \
    if (dec.validate_msg(cmd)) {                      \
      ok = true;                                      \
      log->info("successfully parsed command " #cmd); \
    } else {                                          \
      log->warn("failed to checksum command " #cmd);  \
    }                                                 \
  } else {                                            \
    log->warn("failed to parse command " #cmd);       \
  }

namespace spv {
MODULE_LOGGER

void Connection::connect() {
  log->debug("connecting to peer {}", addr_);
  tcp_->connect(addr_.uvw_addr());
}

void Connection::read(const char* data, size_t sz) {
  log->debug("read {} bytes from peer {}", sz, addr_);
  buf_.append(data, sz);
  for (bool ok = true; ok; ok = read_message())
    ;
}

bool Connection::read_message() {
  if (buf_.size() < HEADER_SIZE) {
    return false;
  }

  // get the headers
  Headers hdrs;
  {
    Decoder dec(buf_.data(), HEADER_SIZE);
    assert(dec.pull(hdrs));
  }

  // do we have enough data for the payload?
  size_t msg_size = HEADER_SIZE + hdrs.payload_size;
  if (buf_.size() < msg_size) {
    return false;
  }

  const std::string command = hdrs.command;
  log->debug("peer {} sent command: {}", addr_, command);

  bool ok = false;
  Decoder dec(buf_.data() + HEADER_SIZE, hdrs.payload_size);
  if (command == "version") {
    Version version;
    PULL_MSG(version);
    if (ok) {
      their_nonce_ = version.nonce;
      their_services_ = version.services;

      Verack ack;
      send_msg(ack);
    }
  } else if (command == "verack") {
    Verack verack;
    PULL_MSG(verack);
  } else if (command == "ping") {
    Ping ping;
    PULL_MSG(ping);
    if (ok) {
      Pong pong(ping.nonce);
      send_msg(pong);
    }
  } else if (command == "pong") {
    Pong pong;
    PULL_MSG(pong);
  }
  buf_.consume(msg_size);
  return true;
}

void Connection::send_msg(const Message& msg) {
  size_t sz;
  std::unique_ptr<char[]> data = msg.encode(sz);
  log->debug("sending message '{}', size={}, addr={}", msg.headers.command, sz,
             addr_);
  tcp_->write(std::move(data), sz);
}

void Connection::send_version() {
  Version ver;
  ver.version = PROTOCOL_VERSION;
  ver.services = services_;
  ver.timestamp = time32();
  ver.addr_recv.addr = addr_;
  ver.nonce = nonce_;
  ver.user_agent = SPV_USER_AGENT;

  send_msg(ver);
}
}  // namespace spv
