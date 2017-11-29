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

#include "./buffer.h"

#include "./logging.h"

namespace spv {
MODULE_LOGGER

void Buffer::ensure_capacity(size_t len) {
  size_t new_capacity = capacity_;
  while (size_ + len > new_capacity) {
    new_capacity *= 2;
  }
  const size_t extra = new_capacity - capacity_;
  if (extra) {
    log->debug("growing buffer from {} to {}", capacity_, new_capacity);
    std::unique_ptr<char[]> new_data(new char[new_capacity]);
    std::memmove(new_data.get(), data_.get(), capacity_);
    std::memset(new_data.get() + capacity_, 0, extra);
    data_ = std::move(new_data);
    capacity_ = new_capacity;
  }
}

std::unique_ptr<char[]> Buffer::move_buffer(size_t &sz) {
  sz = size_;
  size_ = 0;
  capacity_ = 0;
  return std::move(data_);
}
}  // namespace spv
