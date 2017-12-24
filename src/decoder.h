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

#include <cassert>
#include <sstream>
#include <stdexcept>

#include "./fields.h"
#include "./message.h"
#include "./pow.h"

namespace spv {
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

  bool validate_msg(const Message *msg) const;

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

  // XXX: danger, only used to pull accumulated PoW from database
  void pull(double &val) {
    uint64_t ival;
    pull(ival);
    double *pval = reinterpret_cast<double *>(&ival);
    val = *pval;
  }

  void pull(CCode &ccode) {
    uint8_t int_code;
    pull(int_code);
    ccode = static_cast<CCode>(int_code);
  }

  void pull(InvType &inv) {
    uint32_t int_code;
    pull(int_code);
    inv = static_cast<InvType>(int_code);
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

  void pull(Headers &headers);

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

  void pull(hash_t &hash) {
    pull_buf(hash.data(), sizeof hash);
    std::reverse(hash.begin(), hash.end());
  }

  void pull(BlockHeader &hdr, bool pull_tx = true) {
    size_t start = off_;
    pull(hdr.version);
    pull(hdr.prev_block);
    pull(hdr.merkle_root);
    pull(hdr.timestamp);
    pull(hdr.bits);
    pull(hdr.nonce);

    // calculate the hash of this block
    hdr.block_hash = pow_hash(data_ + start, off_ - start, true);

    if (pull_tx) {
      uint8_t tx_count;
      pull(tx_count);
      assert(tx_count == 0);
    }
  }
};
}  // namespace spv
