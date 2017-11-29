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

const static std::chrono::seconds ping_interval(60);

Connection::Connection(const Peer& us, Addr addr, uvw::Loop& loop)
    : us_(us),
      peer_(addr),
      loop_(loop),
      tcp_(loop.resource<uvw::TcpHandle>()),
      ping_nonce_(0) {
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
    log->info("message '{}' from peer {}", cmd, peer_);
    if (cmd == "addr") {
      AddrMsg* addrs = dynamic_cast<AddrMsg*>(msg.get());
      for (const auto& addr : addrs->addrs) {
        log->debug("peer sent us new addr {}", addr.addr);
      }
    } else if (cmd == "getaddr") {
      log->debug("ignoring getaddr");
    } else if (cmd == "getblocks") {
      log->debug("ignoring getblocks");
    } else if (cmd == "getheaders") {
      log->debug("ignoring getheaders");
    } else if (cmd == "mempool") {
      log->debug("ignoring mempool");
    } else if (cmd == "version") {
      Version* ver = dynamic_cast<Version*>(msg.get());
      peer_.nonce = ver->nonce;
      peer_.services = ver->services;
      peer_.user_agent = ver->user_agent;
      peer_.version = ver->version;
      log->info("finished handshake with peer {}", peer_);
      send_msg(VerAck{});

      // set up a ping timer; TODO: require pongs
      ping_ = loop_.resource<uvw::TimerHandle>();
      ping_->once<uvw::ErrorEvent>([](const auto&, auto& timer) {
        log->error("got error from ping timer");
        timer.close();
      });
      ping_->on<uvw::TimerEvent>([this](const auto&, auto&) {
        Ping ping;
        ping.nonce = ping_nonce_ = rand64();
        log->debug("ping timer fired for peer {}, using nonce {}", peer_,
                   ping_nonce_);
        send_msg(ping);

        pong_ = loop_.resource<uvw::TimerHandle>();
        pong_->once<uvw::ErrorEvent>([this](const auto&, auto& timer) {
          log->error("got error from pong timer");
          pong_->close();
        });
        pong_->on<uvw::TimerEvent>([this](const auto&, auto&) {
          log->warn("peer did not send up pong in time");
          shutdown();
        });
        pong_->start(std::chrono::seconds(5), std::chrono::seconds(0));
      });
      ping_->start(ping_interval, ping_interval);
    } else if (cmd == "verack") {
      log->debug("ignoring verack");
    } else if (cmd == "ping") {
      Ping* ping = dynamic_cast<Ping*>(msg.get());
      Pong pong;
      pong.nonce = ping->nonce;
      send_msg(pong);
    } else if (cmd == "pong") {
      Pong* pong = dynamic_cast<Pong*>(msg.get());
      if (pong_) {
        if (pong->nonce != ping_nonce_) {
          log->warn(
              "peer {} sent invalid pong nonce, they sent {}, we expected {}, "
              "shutting down",
              peer_, pong->nonce, ping_nonce_);
          shutdown();
        } else {
          log->debug("peer {} sent us correct pong nonce!", peer_);
          pong_->close();
        }
        pong_.reset();
      } else {
        log->warn("peer {} sent pong when one was not expected", peer_);
        shutdown();
      }
    } else if (cmd == "reject") {
      Reject* rej = dynamic_cast<Reject*>(msg.get());
      uint8_t ccode = static_cast<uint8_t>(rej->ccode);
      log->error("peer {} sent us reject: message={}, ccode={}, reason={}",
                 peer_, rej->message, ccode, rej->reason);
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

void Connection::shutdown() { log->error("shutdown not yet handled"); }
}  // namespace spv
