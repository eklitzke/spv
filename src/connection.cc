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

namespace spv {
MODULE_LOGGER

void Connection::connect() {
  log->debug("connecting to peer {}", addr_);
  tcp_->connect(addr_.uvw_addr());
}

void Connection::read(const char* data, size_t sz) {
  log->debug("read {} bytes from peer {}", sz, addr_);
  read_buf_.append(data, sz);

  Decoder dec(read_buf_.data(), read_buf_.size());
  Headers hdrs;
  assert(dec.pull_headers(hdrs));

  std::string command = hdrs.command;
  log->debug("peer {} sent command: {}", addr_, command);
  if (command == "version") {
    Version ver(hdrs);
    if (dec.pull_version(ver)) {
      read_buf_.consume(dec.bytes_read());
      log->info("peer sent us version {}, agent = {}", ver.version,
                ver.user_agent);
    }
  } else if (command == "verack") {
    Verack ack(hdrs);
    log->info("peer sent us verack message");
  }
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
  std::unique_ptr<char[]> data = enc.move_buffer(&sz);
  log->debug("writing {} byte version message to {}", sz, addr_);
  tcp_->write(std::move(data), sz);
}
}  // namespace spv
