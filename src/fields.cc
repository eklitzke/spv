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

#include "./fields.h"

#include <algorithm>
#include <cassert>

#include "./decoder.h"
#include "./encoder.h"

std::ostream &operator<<(std::ostream &o, const spv::BlockHeader &hdr) {
  struct tm *tmp;
  time_t ts = static_cast<time_t>(hdr.timestamp);
  tmp = localtime(&ts);
  assert(tmp != nullptr);
  char time_buf[200];
  assert(strftime(time_buf, sizeof time_buf, "%Y-%m-%d %H:%M:%S", tmp) != 0);

  o << "BlockHeader(hash=" << hdr.block_hash << " time=" << time_buf;
  if (hdr.height) {
    o << " height=" << hdr.height;
  }
  return o << ")";
}

std::ostream &operator<<(std::ostream &o, const spv::NetAddr &addr) {
  return o << "NetAddr(addr=" << addr.addr << ", services=" << addr.services
           << ")";
}

namespace spv {
std::string BlockHeader::db_encode() const {
  Encoder enc;
  enc.push(*this, false);
  enc.push(this->height);

  size_t sz;
  std::unique_ptr<char[]> data = enc.serialize(sz, false);
  return {data.get(), sz};
}

BlockHeader BlockHeader::genesis() {
  BlockHeader hdr;
  Decoder dec(reinterpret_cast<const char *>(genesis_block_hdr.data()),
              genesis_block_hdr.size());
  dec.pull(hdr, false);
  const hash_t genesis_hash{0x00, 0x00, 0x00, 0x00, 0x09, 0x33, 0xea, 0x01,
                            0xad, 0x0e, 0xe9, 0x84, 0x20, 0x97, 0x79, 0xba,
                            0xae, 0xc3, 0xce, 0xd9, 0x0f, 0xa3, 0xf4, 0x08,
                            0x71, 0x95, 0x26, 0xf8, 0xd7, 0x7f, 0x49, 0x43};
  assert(hdr.block_hash == genesis_hash);
  return hdr;
}

void BlockHeader::db_decode(const std::string &s) {
  Decoder dec(s.c_str(), s.size());
  dec.pull(*this, false);
  dec.pull(height);
  assert(!dec.bytes_remaining());
}
}
