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

#include <cstdint>
#include <ostream>

#include "uvw/src/uvw.hpp"

std::ostream& operator<<(std::ostream& o, const uvw::Addr& addr);

namespace spv {
bool operator==(const uvw::Addr& a, const uvw::Addr& b);
}  // namespace spv

namespace std {
template <>
struct hash<uvw::Addr> {
  std::size_t operator()(const uvw::Addr& addr) const noexcept {
    std::size_t h1 = std::hash<std::string>{}(addr.ip);
    std::size_t h2 = std::hash<unsigned int>{}(addr.port);
    return h1 ^ (h2 << 1);
  }
};
}  // namespace std
