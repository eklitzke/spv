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

#include "./util.h"

#include <limits>
#include <random>

namespace {
std::random_device rd;
std::uniform_int_distribution<uint64_t> dist(
    0, std::numeric_limits<uint64_t>::max());
}

namespace spv {
std::mt19937_64 rg(rd());

uint64_t rand64() { return dist(rg); }

std::string to_hex(const char* data, size_t nbytes) {
  std::string output;
  output.reserve(2 * nbytes);
  for (size_t i = 0; i < nbytes; i++) {
    hex_encode(*(data + i), output);
  }
  return output;
}

std::string to_hex(const std::string& str) {
  std::string output;
  output.reserve(2 * str.size());
  for (const auto& c : str) {
    hex_encode(c, output);
  }
  return output;
}
}
