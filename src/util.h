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

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>

namespace spv {
extern std::mt19937_64 g;

// convert a raw byte string to hex
std::string string_to_hex(const std::string& input);

// generate a random uint64_t value
uint64_t rand64();

template <typename T>
void shuffle(T& iterable) {
  std::shuffle(iterable.begin(), iterable.end(), g);
}
}  // namespace spv
