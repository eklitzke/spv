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

#include "./encoder.h"

namespace spv {
std::string db_encode(const BlockHeader &hdr) {
  Encoder enc;
  enc.push(hdr, false);
  enc.push(hdr.height);
  enc.push(hdr.block_hash);

  size_t sz;
  std::unique_ptr<char[]> data = enc.serialize(sz, false);
  return {data.get(), sz};
}
}
