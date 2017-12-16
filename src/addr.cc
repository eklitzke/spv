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

#include "./addr.h"

#include <arpa/inet.h>
#include <cassert>
#include <cstring>

#include "./logging.h"
#include "./settings.h"

namespace spv {
MODULE_LOGGER
Addr::Addr(const addrinfo *ai) : af_(ai->ai_family) {
  assert(ai->ai_family == ai->ai_addr->sa_family);

  char buf[INET6_ADDRSTRLEN];
  std::memset(buf, 0, sizeof buf);

  switch (af_) {
    case AF_INET: {
      sockaddr_in *sa4 = reinterpret_cast<sockaddr_in *>(ai->ai_addr);
      if (inet_ntop(sa4->sin_family, &sa4->sin_addr, buf, sizeof buf) ==
          nullptr) {
        log->error("inet_ntop failed: {}", strerror(errno));
      }
      break;
    }
    case AF_INET6: {
      sockaddr_in6 *sa6 = reinterpret_cast<sockaddr_in6 *>(ai->ai_addr);
      if (inet_ntop(sa6->sin6_family, &sa6->sin6_addr, buf, sizeof buf) ==
          nullptr) {
        log->error("inet_ntop failed: {}", strerror(errno));
      }
      break;
    }
    default:
      log->warn("unknown address family {}", ai->ai_addr->sa_family);
      return;
  }
  port_ = get_settings().port;
  ip_ = buf;
  assert(ip_.size() < INET6_ADDRSTRLEN);
}

const static std::array<uint8_t, 12> ipv4_prefix = {0, 0, 0, 0, 0,    0,
                                                    0, 0, 0, 0, 0xff, 0xff};

void Addr::encode_addrbuf(addrbuf_t &buf) const {
  switch (af_) {
    case -1:
      std::memset(buf.data(), 0, 16);
      static_assert(sizeof(buf) == 16);
      return;
    case AF_INET:
      std::memmove(buf.data(), ipv4_prefix.data(), 12);
      std::memmove(buf.data() + 12, &inaddr_.ipv4.s_addr, 4);
      static_assert(sizeof(inaddr_.ipv4.s_addr) == 4);
      return;
    case AF_INET6:
      std::memmove(buf.data(), &inaddr_.ipv6.s6_addr, 16);
      static_assert(sizeof(inaddr_.ipv6.s6_addr) == 16);
      return;
  }
  assert(false);  // not reached
}

void Addr::set_addr(const addrbuf_t &buf) {
  // apply a basic heuristic to detect ipv4 messages
  const void *src;
  if (std::memcmp(buf.data(), ipv4_prefix.data(), 12) == 0) {
    af_ = AF_INET;
    src = buf.data() + 12;
  } else {
    af_ = AF_INET6;
    src = buf.data();
  }
  char string_buf[INET6_ADDRSTRLEN];
  const char *s = inet_ntop(af_, src, string_buf, sizeof string_buf);
  if (s == nullptr) {
    log->warn("failed to decode addr: {}", strerror(errno));
    return;
  }
  ip_ = s;
  assert(ip_.size() < INET6_ADDRSTRLEN);
}
}  // namespace spv

std::ostream &operator<<(std::ostream &o, const spv::Addr &addr) {
  return o << addr.ip() << ":" << addr.port();
}
