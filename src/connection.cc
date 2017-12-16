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

#include "./client.h"
#include "./constants.h"
#include "./logging.h"
#include "./message.h"
#include "./uvw.h"

namespace spv {
MODULE_LOGGER

const static std::chrono::seconds ping_interval(60);

inline void toggle_on(bool& value) {
  assert(!value);
  value = true;
}

Connection::Connection(Client* client, const Addr& addr)
    : loop_(client->loop_),
      client_(client),
      peer_(addr),
      have_version_(false),
      have_verack_(false),
      tcp_(client->loop_->resource<uvw::TcpHandle>()),
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
#if 0
  log->debug("read {} bytes from peer {}", sz, peer_);
#endif
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

  // TODO: use a hash table for this, like in the decoder
  if (msg.get() != nullptr) {
    const std::string& cmd = msg->headers.command;
    log->debug("message '{}' from peer {}", cmd, peer_);

    if (cmd != "version" && cmd != "verack" && !connected()) {
      log->error(
          "unexpectedly received message '{}' from peer {} in unconnected "
          "state, have_version = {}, have_verack = {}",
          cmd, peer_, have_version_, have_verack_);
      client_->notify_error(this, "protocol error");
      return ret;
    }

    if (cmd == "addr") {
      handle_addr(dynamic_cast<AddrMsg*>(msg.get()));
    } else if (cmd == "getaddr") {
      handle_getaddr(dynamic_cast<GetAddr*>(msg.get()));
    } else if (cmd == "getblocks") {
      handle_getblocks(dynamic_cast<GetBlocks*>(msg.get()));
    } else if (cmd == "getheaders") {
      handle_getheaders(dynamic_cast<GetHeaders*>(msg.get()));
    } else if (cmd == "headers") {
      handle_headers(dynamic_cast<HeadersMsg*>(msg.get()));
    } else if (cmd == "inv") {
      handle_inv(dynamic_cast<Inv*>(msg.get()));
    } else if (cmd == "mempool") {
      handle_mempool(dynamic_cast<Mempool*>(msg.get()));
    } else if (cmd == "ping") {
      handle_ping(dynamic_cast<Ping*>(msg.get()));
    } else if (cmd == "pong") {
      handle_pong(dynamic_cast<Pong*>(msg.get()));
    } else if (cmd == "reject") {
      handle_reject(dynamic_cast<Reject*>(msg.get()));
    } else if (cmd == "sendheaders") {
      handle_sendheaders(dynamic_cast<SendHeaders*>(msg.get()));
    } else if (cmd == "verack") {
      handle_verack(dynamic_cast<VerAck*>(msg.get()));
    } else if (cmd == "version") {
      handle_version(dynamic_cast<Version*>(msg.get()));
    } else {
      handle_unknown(cmd);
    }
  }
  return ret;
}

void Connection::send_msg(const Message& msg) {
  size_t sz;
  std::unique_ptr<char[]> data = msg.encode(sz);
  const std::string& cmd = msg.headers.command;
  log->debug("sending '{}' to {}", cmd, peer_);
  tcp_->write(std::move(data), sz);
}

void Connection::send_version() {
  Version ver;
  ver.version = PROTOCOL_VERSION;
  ver.services = client_->us_.services;
  ver.addr_recv.addr = peer_.addr;
  ver.nonce = client_->us_.nonce;
  ver.user_agent = client_->us_.user_agent;
  send_msg(ver);

  // expect a verack msg within 5 seconds
  verack_ = client_->loop_->resource<uvw::TimerHandle>();
  verack_->once<uvw::ErrorEvent>([](const auto&, auto& timer) {
    log->error("got error from verack timer");
    timer.close();
  });
  verack_->on<uvw::TimerEvent>([this](const auto&, auto& timer) {
    client_->notify_error(this, "verack timeout");
    timer.close();
  });
  verack_->start(std::chrono::seconds(5), std::chrono::seconds(0));
}

void Connection::get_headers(const std::vector<hash_t>& locator_hashes,
                             const hash_t& hash_stop) {
  GetHeaders req;
  req.version = client_->us_.version;
  req.locator_hashes = locator_hashes;
  req.hash_stop = hash_stop;
  send_msg(req);
}

void Connection::get_headers(const BlockHeader& start_hdr) {
  log->info("fetching headers from peer {} starting at block {}", peer_,
            start_hdr);
  std::vector<hash_t> needed{start_hdr.block_hash};
  return get_headers(needed);
}

void Connection::shutdown() {
  bool did_shutdown = false;
  if (ping_) {
    ping_->stop();
    ping_->close();
    ping_.reset();
    did_shutdown = true;
  }
  if (pong_) {
    pong_->stop();
    pong_->close();
    pong_.reset();
    did_shutdown = true;
  }
  if (tcp_) {
    tcp_->close();
    tcp_.reset();
    did_shutdown = true;
  }
  if (did_shutdown) {
    log->debug("shutdown connection to peer {}", peer_);
  }
}

void Connection::handle_addr(AddrMsg* addrs) {
  bool new_peers = false;
  for (const auto& addr : addrs->addrs) {
    client_->notify_peer(addr);
    if (addr.addr != peer_.addr) {
      new_peers = true;
    }
  }
  if (new_peers && getaddr_) {
    getaddr_->stop();
    getaddr_->close();
    getaddr_.reset();
  }
}
void Connection::handle_getaddr(GetAddr* addr) {
  log->debug("ignoring getaddr message");
}

void Connection::handle_getblocks(GetBlocks* blocks) {
  log->debug("ignoring getblocks message");
}

void Connection::handle_getheaders(GetHeaders* headers) {
  log->debug("ignoring getheaders message");
}

void Connection::handle_headers(HeadersMsg* msg) {
  log->debug("headers message with {} block headers",
             msg->block_headers.size());
  client_->notify_headers(msg->block_headers);
}

void Connection::handle_mempool(Mempool* pool) {
  log->debug("ignoring mempool message");
}

void Connection::handle_inv(Inv* inv) {
  log->info("inv message with type {}, hash {}", inv->type, to_hex(inv->hash));
}

void Connection::handle_ping(Ping* ping) {
  Pong pong;
  pong.nonce = ping->nonce;
  send_msg(pong);
}

void Connection::handle_pong(Pong* pong) {
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
}

void Connection::handle_reject(Reject* rej) {
  uint8_t ccode = static_cast<uint8_t>(rej->ccode);
  log->error("peer {} sent us reject: message={}, ccode={}, reason={}", peer_,
             rej->message, ccode, rej->reason);
}

void Connection::handle_sendheaders(SendHeaders* send) {
  log->debug("ignoring sendheaders message");
}

void Connection::handle_unknown(const std::string& cmd) {
  log->error("decoder returned unknown p2p message '{}'", cmd);
}

void Connection::handle_verack(VerAck* ack) {
  toggle_on(have_verack_);
  assert(verack_);
  verack_->stop();
  verack_->close();
  verack_.reset();
}

void Connection::handle_version(Version* ver) {
  toggle_on(have_version_);

  peer_.nonce = ver->nonce;
  peer_.services = ver->services;
  peer_.user_agent = ver->user_agent;
  peer_.version = ver->version;
  peer_.time = now();
  log->info("finished handshake with peer {}", peer_);
  send_msg(VerAck{});  // send required verack
  get_new_addrs();     // ask for more peers

  // set up a ping timer
  ping_ = client_->loop_->resource<uvw::TimerHandle>();
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

    pong_ = client_->loop_->resource<uvw::TimerHandle>();
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

  // tell the client that we're ready to fetch headers
  client_->notify_connected(this);
}

void Connection::get_new_addrs() {
  assert(!getaddr_);
  getaddr_ = client_->loop_->resource<uvw::TimerHandle>();
  getaddr_->once<uvw::ErrorEvent>([](const auto&, auto& timer) {
    log->error("got error from getaddr timer");
    timer.close();
  });
  getaddr_->on<uvw::TimerEvent>([this](const auto&, auto& timer) {
    log->info(
        "peer {} failed to respond to getaddr, asking client to connect to "
        "new seed peer",
        peer_);
    client_->connect_to_new_peer();
    timer.close();
  });
  getaddr_->start(std::chrono::seconds(5), std::chrono::seconds(0));
}

}  // namespace spv
