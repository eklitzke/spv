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
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <random>
#include <string>

namespace spv {
typedef std::chrono::time_point<std::chrono::system_clock> time_point;

extern std::mt19937_64 g;

inline time_point now() { return std::chrono::system_clock::now(); }

// convert an array to hex (for debubbing/logging)
template <size_t N>
std::string array_to_hex(const std::array<uint8_t, N>& arr) {
  static const char* const lut = "0123456789abcdef";

  std::string output;
  output.reserve(2 * N);
  for (const auto& c : arr) {
    output.push_back(lut[c >> 4]);
    output.push_back(lut[c & 15]);
  }
  return output;
}

// generate a random uint64_t value
uint64_t rand64();

inline uint32_t time32() {
  time_t tv = time(nullptr);
  return static_cast<uint32_t>(tv);
}

inline uint64_t time64() {
  time_t tv = time(nullptr);
  return static_cast<uint64_t>(tv);
}

template <typename T>
void shuffle(T& iterable) {
  std::shuffle(iterable.begin(), iterable.end(), g);
}
}  // namespace spv
