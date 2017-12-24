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
#include <iostream>

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
    o << " height=" << hdr.height << ", log2_work = " << hdr.accumulated_work;
  }
  return o << ")";
}

std::ostream &operator<<(std::ostream &o, const spv::NetAddr &addr) {
  return o << "NetAddr(addr=" << addr.addr << ", services=" << addr.services
           << ")";
}

inline double log2(double val) { return log(val) / log(2.0); }
inline double pow2(double val) { return pow(2, val); }

namespace spv {
std::string BlockHeader::db_encode() const {
  Encoder enc;
  enc.push(*this, false);
  enc.push(this->height);
  enc.push(this->accumulated_work);

  size_t sz;
  std::unique_ptr<char[]> data = enc.serialize(sz, false);
  return {data.get(), sz};
}

double BlockHeader::work_required() const {
  uint32_t low_bits = bits & 0xffffff;
  uint32_t high_bits = (bits & 0xff000000) >> 24;
  double difficulty = low_bits * pow(2, 8 * (high_bits - 3));
  std::cerr << "bits = " << bits << ", low_bits = " << low_bits
            << ", high_bits = " << high_bits << ", difficulty = " << difficulty
            << std::endl;

  return difficulty * (1UL << 48) / 0xffff;
}

BlockHeader BlockHeader::genesis() {
  BlockHeader hdr;
  Decoder dec(reinterpret_cast<const char *>(genesis_block_hdr.data()),
              genesis_block_hdr.size());
  dec.pull(hdr, false);
  assert(hdr.is_genesis());
  double req = hdr.work_required();
  std::cerr << "work required: " << req << std::endl;
  hdr.accumulated_work = hdr.work_required();
  return hdr;
}

void BlockHeader::daisy_chain(const BlockHeader &prev) {
  assert(height == 0);
  assert(prev.block_hash == prev_block);
  assert(prev.height > 0 || prev.is_genesis());
  assert(prev.accumulated_work > 0);

  height = prev.height + 1;
  accumulated_work = log2(work_required() + pow2(prev.accumulated_work));
  assert(accumulated_work >= prev.accumulated_work);
}

void BlockHeader::db_decode(const std::string &s) {
  Decoder dec(s.c_str(), s.size());
  dec.pull(*this, false);
  dec.pull(height);
  dec.pull(accumulated_work);
  assert(!dec.bytes_remaining());
}

uint32_t BlockHeader::age() const {
  uint32_t now = time32();
  if (now <= timestamp) {
    return 0;
  }
  return now - timestamp;
}
}
