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

#include <exception>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "./constants.h"
#include "./logging.h"
#include "./message.h"
#include "./pow.h"

namespace spv {
MODULE_LOGGER
struct DecodeError : std::runtime_error {
  DecodeError(const std::string &why) : std::runtime_error(why) {}
};

struct IncompleteParse : DecodeError {
  IncompleteParse(const std::string &why) : DecodeError(why) {}
};

struct UnknownMessage : DecodeError {
  UnknownMessage(const std::string &why) : DecodeError(why) {}
};

struct BadMessage : DecodeError {
  BadMessage(const std::string &why) : DecodeError(why) {}
};

struct Decoder {
  const char *data_;
  size_t cap_;
  size_t off_;

  Decoder() = delete;
  Decoder(const char *data, size_t sz) : data_(data), cap_(sz), off_(0) {}

  void reset(const char *data, size_t sz) {
    data_ = data;
    cap_ = sz;
    off_ = 0;
  }

  inline size_t bytes_remaining() { return cap_ - off_; }

  bool validate_msg(const Message *msg) {
    if (msg == nullptr) {
      return false;
    }
    if (off_ != cap_) {
      log->warn("failed to pull enough bytes");
      return false;
    }
    uint32_t cksum = checksum(data_, cap_);
    if (cksum != msg->headers.checksum) {
      log->warn("invalid checksum!");
      return false;
    }
    return true;
  }

  inline void pull_buf(void *out, size_t sz) {
    if (sz + off_ > cap_) {
      std::ostringstream os;
      os << "failed to pull " << sz << " bytes, offset = " << off_
         << ", capacity = " << cap_;
      throw IncompleteParse(os.str());
    }
    std::memmove(out, data_ + off_, sz);
    off_ += sz;
  }

  inline void pull_varint(uint64_t &out) {
    uint8_t prefix;
    pull(prefix);
    if (prefix < 0xfd) {
      out = prefix;
      return;
    }
    switch (prefix) {
      case 0xfd: {
        uint16_t val;
        pull(val);
        out = val;
        return;
      }
      case 0xfe: {
        uint32_t val;
        pull(val);
        out = val;
        return;
      }
      case 0xff:
        pull(out);
        return;
    }
    assert(false);  // not reached
  }

  void pull(uint8_t &out) { pull_buf(&out, sizeof out); }

  void pull(uint16_t &out) {
    pull_buf(&out, sizeof out);
    out = le16toh(out);
  }

  void pull(uint32_t &out) {
    pull_buf(&out, sizeof out);
    out = le32toh(out);
  }

  void pull(uint64_t &out) {
    pull_buf(&out, sizeof out);
    out = le64toh(out);
  }

  void pull_be(uint16_t &out) {
    pull_buf(&out, sizeof out);
    out = be16toh(out);
  }

  void pull(CCode &ccode) {
    uint8_t int_code;
    pull(int_code);
    ccode = static_cast<CCode>(int_code);
  }

  void pull(std::string &out) {
    uint64_t sz;
    pull_varint(sz);
    if (sz >= std::numeric_limits<uint16_t>::max()) {
      std::ostringstream os;
      os << "string size " << sz << " exceeds valid range";
      throw BadMessage(os.str());
    }
    out.append(data_ + off_, sz);
    off_ += sz;
  }

  void pull(Headers &headers) {
    std::array<char, COMMAND_SIZE> cmd_buf{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pull(headers.magic);
    if (headers.magic != PROTOCOL_MAGIC) {
      log->warn("peer sent wrong magic bytes");
    }
    pull_buf(cmd_buf.data(), COMMAND_SIZE);
    if (cmd_buf[COMMAND_SIZE - 1] != '\0') {
      throw BadMessage("command is not null terminated");
    }
    headers.command = cmd_buf.data();
    pull(headers.payload_size);
    pull(headers.checksum);
  }

  void pull(Addr &addr) {
    // TODO: need to actually use addr_buf
    addrbuf_t addr_buf{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pull_buf(addr_buf.data(), ADDR_SIZE);
    addr.set_addr(addr_buf);

    // port is in network byte order
    uint16_t port;
    pull_be(port);
    addr.set_port(port);
  }

  void pull(VersionNetAddr &addr) {
    pull(addr.services);
    pull(addr.addr);
  }

  void pull(NetAddr &addr) {
    pull(addr.time);
    pull(addr.services);
    pull(addr.addr);
  }

  void pull(hash_t &hash) { pull_buf(hash.data(), sizeof hash); }
};

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
    hash_t hash;
    dec.pull(hash);
    msg->block_headers.push_back(hash);
  }
  return msg;
});

DECLARE_PARSER(inv, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<Inv>(hdrs);
  dec.pull(msg->type);
  dec.pull(msg->hash);
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
  log->debug("pulled headers for command '{}', payload size {}, total_size {}",
             hdrs.command, hdrs.payload_size, total_size);
  if (total_size > size) {
    std::ostringstream os;
    os << "payload for message '" << hdrs.command << "' not ready yet, we need "
       << total_size << " bytes, we have " << size << " bytes";
    throw IncompleteParse(os.str());
  }

  *bytes_consumed = total_size;
  dec.reset(data + HEADER_SIZE, hdrs.payload_size);
  log->debug("pulling {} byte payload for command '{}'", hdrs.payload_size,
             hdrs.command);

  return registry_.parse(dec, hdrs);
}

std::unique_ptr<Message> decode_message(const char *data, size_t size,
                                        size_t *bytes_consumed) {
  try {
    return internal_decode_message(data, size, bytes_consumed);
  } catch (const IncompleteParse &exc) {
    log->debug("incomplete parse: {}", exc.what());
  } catch (const UnknownMessage &exc) {
    log->warn("unhandled p2p message: '{}'", exc.what());
  } catch (const BadMessage &exc) {
    log->warn("bad p2p message parse: {}", exc.what());
  }
  return nullptr;
}

}  // namespace spv
