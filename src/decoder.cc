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

#include "./decoder.h"

#include "./logging.h"

namespace spv {
MODULE_LOGGER

bool Decoder::validate_msg(const Message *msg) const {
  if (msg == nullptr) {
    return false;
  }
  if (off_ != cap_) {
    log->warn("failed to pull enough bytes");
    return false;
  }
  uint32_t cksum = checksum(data_, cap_);
  if (cksum != msg->headers.checksum) {
    log->warn("invalid checksum!");
    return false;
  }
  return true;
}

void Decoder::pull(Headers &headers) {
  std::array<char, COMMAND_SIZE> cmd_buf{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  pull(headers.magic);
  if (headers.magic != PROTOCOL_MAGIC) {
    log->warn("peer sent wrong magic bytes");
  }
  pull_buf(cmd_buf.data(), COMMAND_SIZE);
  if (cmd_buf[COMMAND_SIZE - 1] != '\0') {
    throw BadMessage("command is not null terminated");
  }
  headers.command = cmd_buf.data();
  pull(headers.payload_size);
  pull(headers.checksum);
}
}
