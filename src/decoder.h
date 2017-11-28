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

#pragma once

#include <endian.h>

#include "./protocol.h"
#include "./reader.h"

namespace spv {
class Decoder {
 public:
  Decoder() = delete;
  Decoder(const char *data, size_t sz) : rd_(data, sz) {}

  bool pull_headers(Headers &headers) {
    if (!rd_.pull(headers)) return false;
    headers.payload_size = le32toh(headers.payload_size);
    return true;
  }

 private:
  Reader rd_;
};
}  // namespace spv
