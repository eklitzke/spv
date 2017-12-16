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

#include "./message.h"

#include <endian.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#include "./addr.h"
#include "./buffer.h"
#include "./constants.h"
#include "./decoder.h"
#include "./encoder.h"
#include "./logging.h"
#include "./peer.h"
#include "./pow.h"

#define DECLARE_ENCODE(cls) \
  std::unique_ptr<char[]> cls::encode(size_t &sz) const

namespace spv {
MODULE_LOGGER

DECLARE_ENCODE(AddrMsg) {
  Encoder enc(headers);
  enc.push_varint(addrs.size());
  for (const auto &addr : addrs) {
    enc.push(addr);
  }
  return enc.serialize(sz);
}

DECLARE_ENCODE(GetAddr) { return Encoder(headers).serialize(sz); }

DECLARE_ENCODE(GetBlocks) {
  Encoder enc(headers);
  enc.push(version);
  enc.push_varint(locator_hashes.size());
  for (const auto &locator : locator_hashes) {
    enc.push(locator);
  }
  enc.push(hash_stop);
  return enc.serialize(sz);
}

DECLARE_ENCODE(GetHeaders) {
  Encoder enc(headers);
  enc.push(version);
  enc.push_varint(locator_hashes.size());
  for (const auto &locator : locator_hashes) {
    enc.push(locator);
  }
  enc.push(hash_stop);
  return enc.serialize(sz);
}

DECLARE_ENCODE(HeadersMsg) {
  Encoder enc(headers);
  enc.push_varint(block_headers.size());
  for (const auto &hdr : block_headers) {
    enc.push(hdr);
  }
  return enc.serialize(sz);
}

DECLARE_ENCODE(Inv) {
  Encoder enc(headers);
  enc.push_varint(invs.size());
  for (const auto &pr : invs) {
    enc.push(pr.first);
    enc.push(pr.second);
  }
  return enc.serialize(sz);
}

DECLARE_ENCODE(Mempool) { return Encoder(headers).serialize(sz); }

DECLARE_ENCODE(Ping) {
  Encoder enc(headers);
  enc.push(nonce);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Pong) {
  Encoder enc(headers);
  enc.push(nonce);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Reject) {
  Encoder enc(headers);
  enc.push(message);
  enc.push(ccode);
  enc.push(reason);
  if (data != empty_hash) {
    enc.push(data);
  }
  return enc.serialize(sz);
}

DECLARE_ENCODE(SendHeaders) { return Encoder(headers).serialize(sz); }

DECLARE_ENCODE(VerAck) { return Encoder(headers).serialize(sz); }

DECLARE_ENCODE(Version) {
  Encoder enc(headers);
  enc.push(version);
  enc.push(services);
  enc.push(timestamp);
  enc.push(addr_recv);
  enc.push(addr_from);
  enc.push(nonce);
  enc.push(user_agent);
  enc.push(start_height);
  enc.push(relay);
  return enc.serialize(sz);
}

typedef std::function<std::unique_ptr<Message>(Decoder &, const Headers &)>
    handler_t;

class HandlerRegistry {
 public:
  HandlerRegistry() {}
  HandlerRegistry(const HandlerRegistry &other) = delete;

  void add_handler(const std::string &command, handler_t handler) {
    handlers_.insert(std::make_pair(command, handler));
  }

  std::unique_ptr<Message> parse(Decoder &dec, const Headers &hdrs) {
    const std::string &cmd = hdrs.command;
    const auto it = handlers_.find(cmd);
    if (it == handlers_.end()) {
      throw UnknownMessage(cmd);
    }
    return std::unique_ptr<Message>(it->second(dec, hdrs));
  }

 private:
  std::unordered_map<std::string, handler_t> handlers_;
};

static HandlerRegistry registry_;

struct MessageParser {
  MessageParser() = delete;
  MessageParser(const MessageParser &other) = delete;
  MessageParser(const std::string &cmd, handler_t callback) {
    registry_.add_handler(cmd, callback);
  }
};

#define DECLARE_PARSER(cmd, callback) \
  static MessageParser cmd##_parser(#cmd, callback);

DECLARE_PARSER(addr, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<AddrMsg>(hdrs);
  uint64_t count;
  dec.pull_varint(count);
  if (count > 1000) {
    std::ostringstream os;
    os << "addr count " << count << " is too large, ignoring";
    throw BadMessage(os.str());
  }
  log->debug("peer is sending us {} addr(s)", count);
  for (size_t i = 0; i < count; i++) {
    NetAddr addr;
    dec.pull(addr);
    msg->addrs.push_back(addr);
  }
  return msg;
});

DECLARE_PARSER(getaddr, [](auto &dec, const auto &hdrs) {
  return std::make_unique<GetAddr>(hdrs);
});

DECLARE_PARSER(getblocks, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<GetBlocks>(hdrs);
  dec.pull(msg->version);
  uint64_t count;
  dec.pull_varint(count);
  if (count > 2000) {
    std::ostringstream os;
    os << "getblocks hash_count " << count << " is too large, ignoring";
    throw BadMessage(os.str());
  }
  log->debug("peer wants {} block(s)", count);
  for (size_t i = 0; i < count; i++) {
    hash_t locator_hash;
    dec.pull(locator_hash);
    msg->locator_hashes.push_back(locator_hash);
  }
  dec.pull(msg->hash_stop);
  return msg;
});

DECLARE_PARSER(getheaders, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<GetHeaders>(hdrs);
  dec.pull(msg->version);
  uint64_t count;
  dec.pull_varint(count);
  if (count > 2000) {
    std::ostringstream os;
    os << "getheaders hash_count " << count << " is too large, ignoring";
    throw BadMessage(os.str());
  }
  log->debug("peer wants {} header(s)", count);
  for (size_t i = 0; i < count; i++) {
    hash_t locator_hash;
    dec.pull(locator_hash);
    msg->locator_hashes.push_back(locator_hash);
  }
  dec.pull(msg->hash_stop);
  return msg;
});

DECLARE_PARSER(headers, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<HeadersMsg>(hdrs);
  uint64_t count;
  dec.pull_varint(count);
  if (count > 10000) {
    std::ostringstream os;
    os << "headers count " << count << " is too large, ignoring";
    throw BadMessage(os.str());
  }
  for (size_t i = 0; i < count; i++) {
    BlockHeader hdr;
    dec.pull(hdr);
    msg->block_headers.push_back(hdr);
  }
  return msg;
});

DECLARE_PARSER(inv, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<Inv>(hdrs);
  uint64_t count;
  dec.pull_varint(count);
  if (count > 50000) {
    std::ostringstream os;
    os << "inv count " << count << " is too large, ignoring";
    throw BadMessage(os.str());
  }
  for (size_t i = 0; i < count; i++) {
    InvType inv_type;
    hash_t hash;
    dec.pull(inv_type);
    dec.pull(hash);
    msg->invs.emplace_back(inv_type, hash);
  }
  return msg;
});

DECLARE_PARSER(mempool, [](auto &dec, const auto &hdrs) {
  return std::make_unique<Mempool>(hdrs);
});

DECLARE_PARSER(ping, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<Ping>(hdrs);
  dec.pull(msg->nonce);
  return msg;
});

DECLARE_PARSER(pong, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<Pong>(hdrs);
  dec.pull(msg->nonce);
  return msg;
});

DECLARE_PARSER(reject, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<Reject>(hdrs);
  dec.pull(msg->message);
  dec.pull(msg->ccode);
  dec.pull(msg->reason);
  const size_t remaining = dec.bytes_remaining();
  switch (remaining) {
    case 0:
      return msg;
    case sizeof(hash_t):
      dec.pull(msg->data);
      return msg;
    default: {
      std::ostringstream os;
      os << "unable to decode reject message with " << remaining
         << " data bytes";
      throw BadMessage(os.str());
    }
  }
  // this branch not reached
  assert(false);
  return msg;
});

DECLARE_PARSER(sendheaders, [](auto &dec, const auto &hdrs) {
  return std::make_unique<SendHeaders>(hdrs);
});

DECLARE_PARSER(verack, [](auto &dec, const auto &hdrs) {
  return std::make_unique<VerAck>(hdrs);
});

DECLARE_PARSER(version, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<Version>(hdrs);
  dec.pull(msg->version);
  dec.pull(msg->services);
  dec.pull(msg->timestamp);
  dec.pull(msg->addr_recv);
  if (msg->version >= 106) {
    dec.pull(msg->addr_from);
    dec.pull(msg->nonce);
    dec.pull(msg->user_agent);
    dec.pull(msg->start_height);
    if (msg->version >= 70001) {
      dec.pull(msg->relay);
    }
  }
  return msg;
});

static std::unique_ptr<Message> internal_decode_message(
    const char *data, size_t size, size_t *bytes_consumed) {
  Decoder dec(data, size);
  Headers hdrs;
  dec.pull(hdrs);

  size_t total_size = HEADER_SIZE + hdrs.payload_size;
#if 0
  log->debug("pulled headers for command '{}', payload size {}, total_size {}",
             hdrs.command, hdrs.payload_size, total_size);
#endif
  if (total_size > size) {
    std::ostringstream os;
    os << "payload for message '" << hdrs.command << "' not ready yet, we need "
       << total_size << " bytes, we have " << size << " bytes";
    throw IncompleteParse(os.str());
  }

  *bytes_consumed = total_size;
  dec.reset(data + HEADER_SIZE, hdrs.payload_size);
#if 0
  log->debug("pulling {} byte payload for command '{}'", hdrs.payload_size,
             hdrs.command);
#endif

  return registry_.parse(dec, hdrs);
}

std::unique_ptr<Message> decode_message(const char *data, size_t size,
                                        size_t *bytes_consumed) {
  try {
    return internal_decode_message(data, size, bytes_consumed);
  } catch (const IncompleteParse &exc) {
#if 0
    log->debug("incomplete parse: {}", exc.what());
#endif
  } catch (const UnknownMessage &exc) {
    std::string msg(exc.what());
    if (msg != "alert") log->warn("unhandled p2p message: '{}'", msg);
  } catch (const BadMessage &exc) {
    log->warn("bad p2p message parse: {}", exc.what());
  }
  return nullptr;
}
}
