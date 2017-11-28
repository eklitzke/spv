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
  if (dec.pull_##cmd(msg)) {                          \
    if (dec.validate_msg(msg)) {                      \
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
    dec.pull_headers(hdrs);
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
    Version msg(hdrs);
    PULL_MSG(version);
  } else if (command == "verack") {
    Verack msg(hdrs);
    PULL_MSG(verack);
  } else if (command == "ping") {
    Ping msg(hdrs);
    PULL_MSG(ping);
  } else if (command == "pong") {
    Pong msg(hdrs);
    PULL_MSG(pong);
  }
  buf_.consume(msg_size);
  return true;
}

void Connection::send_version(uint64_t nonce, uint64_t services) {
  our_nonce_ = nonce;
  our_services_ = services;

  Encoder enc("version");
  enc.push_int<uint32_t>(PROTOCOL_VERSION);  // version
  enc.push_int<uint64_t>(services);          // services
  enc.push_time<uint64_t>();                 // timestamp
  enc.push_netaddr(addr_);                   // addr_recv
  enc.push_netaddr(nullptr);                 // addr_from
  enc.push_int<uint64_t>(nonce);             // nonce
  enc.push_string(SPV_USER_AGENT);           // user-agent
  enc.push_int<uint32_t>(0);                 // start height
  enc.push_bool(true);                       // relay

  size_t sz;
  std::unique_ptr<char[]> data = enc.serialize(&sz);
  log->debug("writing {} byte version message to {}", sz, addr_);
  tcp_->write(std::move(data), sz);
}
}  // namespace spv
