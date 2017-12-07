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

#include "./spv.pb.h"

std::ostream &operator<<(std::ostream &o, const spv::BlockHeader &hdr) {
  struct tm *tmp;
  time_t ts = static_cast<time_t>(hdr.timestamp);
  tmp = localtime(&ts);
  assert(tmp != nullptr);
  char out[200];
  assert(strftime(out, sizeof out, "%Y-%m-%d %H:%M:%S", tmp) != 0);

  return o << "BlockHeader(hash=" << hdr.block_hash << " time=" << out << ")";
}

namespace spv {
std::string BlockHeader::to_proto() const {
  proto::BlockHeader pb_hdr;
  pb_hdr.set_version(version);
  pb_hdr.set_prev_block(prev_block.data(), sizeof(hash_t));
  pb_hdr.set_prev_block(merkle_root.data(), sizeof(hash_t));
  pb_hdr.set_timestamp(timestamp);
  pb_hdr.set_difficulty(difficulty);
  pb_hdr.set_nonce(nonce);
  pb_hdr.set_height(height);
  pb_hdr.set_block_hash(block_hash.data(), sizeof(hash_t));
  std::string out;
  pb_hdr.SerializeToString(&out);
  return out;
}
}
