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
#include "./logging.h"
#include "./peer.h"
#include "./pow.h"

#define DECLARE_ENCODE(cls) \
  std::unique_ptr<char[]> cls::encode(size_t &sz) const

namespace spv {
MODULE_LOGGER

struct Encoder : Buffer {
  Encoder() : Buffer() {}

  template <typename T>
  void push_int(T val) {
    push(val);
  }

  void push(uint8_t val) { append(&val, sizeof val); }

  void push_zeros(size_t count) {}

  void push(uint16_t val) {
    uint16_t enc_val = htole16(val);
    append(&enc_val, sizeof enc_val);
  }

  void push(uint32_t val) {
    uint32_t enc_val = htole32(val);
    append(&enc_val, sizeof enc_val);
  }

  void push(uint64_t val) {
    uint64_t enc_val = htole64(val);
    append(&enc_val, sizeof enc_val);
  }

  void push(const Headers &headers) {
    push(headers.magic);
    append_string(headers.command, COMMAND_SIZE);
    push(headers.payload_size);
    push(headers.checksum);
  }

  // push in network byte order
  void push_be(uint16_t val) {
    uint16_t enc_val = htobe16(val);
    append(&enc_val, sizeof enc_val);
  }

  void push(const Addr &addr) {
    std::array<char, ADDR_SIZE> buf;
    addr.fill_addr_buf(buf);
    append(buf.data(), ADDR_SIZE);
    push_be(addr.port());
  }

  void push(const VersionNetAddr &addr) {
    push(addr.services);
    push(addr.addr);
  }

  void push(const NetAddr &addr) {
    push(addr.time);
    push(addr.services);
    push(addr.addr);
  }

  void push_varint(size_t val) {
    if (val < 0xfd) {
      push_int<uint8_t>(val);
      return;
    } else if (val <= 0xffff) {
      push_int<uint8_t>(0xfd);
      push_int<uint16_t>(val);
      return;
    } else if (val <= 0xffffffff) {
      push_int<uint8_t>(0xfe);
      push_int<uint32_t>(val);
      return;
    }
    push_int<uint8_t>(0xff);
    push_int<uint64_t>(val);
  }

  void push(const std::string &s) {
    push_varint(s.size());
    append(s.c_str(), s.size());
  }

  void finish_headers() {
    // insert the length
    uint32_t len = htole32(size() - HEADER_SIZE);
    insert(&len, sizeof len, HEADER_LEN_OFFSET);

    // insert the checksum
    std::array<char, 4> cksum{0, 0, 0, 0};
    checksum(data() + HEADER_SIZE, size() - HEADER_SIZE, cksum);
    insert(&cksum, sizeof cksum, HEADER_CHECKSUM_OFFSET);
  }

  std::unique_ptr<char[]> serialize(size_t &sz, bool finish = true) {
    if (finish) finish_headers();
    return move_buffer(sz);
  }
};

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
    std::array<char, ADDR_SIZE> addr_buf{0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0};
    pull_buf(addr_buf.data(), ADDR_SIZE);

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

DECLARE_PARSER(sendheaders, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<SendHeaders>(hdrs);
  return msg;
});

DECLARE_PARSER(verack, [](auto &dec, const auto &hdrs) {
  auto msg = std::make_unique<VerAck>(hdrs);
  return msg;
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
    const Peer &peer, const char *data, size_t size, size_t *bytes_consumed) {
  Decoder dec(data, size);
  Headers hdrs;
  dec.pull(hdrs);

  size_t total_size = HEADER_SIZE + hdrs.payload_size;
  log->debug(
      "pulled headers for command '{}', payload size {}, total_size {}, peer "
      "{}",
      hdrs.command, hdrs.payload_size, total_size, peer);
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

std::unique_ptr<Message> decode_message(const Peer &peer, const char *data,
                                        size_t size, size_t *bytes_consumed) {
  try {
    return internal_decode_message(peer, data, size, bytes_consumed);
  } catch (const IncompleteParse &exc) {
    log->debug("incomplete parse: {}", exc.what());
  } catch (const UnknownMessage &exc) {
    log->warn("unhandled p2p message: '{}'", exc.what());
  } catch (const BadMessage &exc) {
    log->warn("bad p2p message parse: {}", exc.what());
  }
  return nullptr;
}

DECLARE_ENCODE(AddrMsg) {
  Encoder enc;
  enc.push_varint(addrs.size());
  for (const auto &addr : addrs) {
    enc.push(addr);
  }
  return enc.serialize(sz);
}

DECLARE_ENCODE(Ping) {
  Encoder enc;
  enc.push(headers);
  enc.push(nonce);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Pong) {
  Encoder enc;
  enc.push(headers);
  enc.push(nonce);
  return enc.serialize(sz);
}

DECLARE_ENCODE(SendHeaders) {
  Encoder enc;
  enc.push(headers);
  return enc.serialize(sz);
}

DECLARE_ENCODE(VerAck) {
  Encoder enc;
  enc.push(headers);
  return enc.serialize(sz);
}

DECLARE_ENCODE(Version) {
  Encoder enc;
  enc.push(headers);
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
}
