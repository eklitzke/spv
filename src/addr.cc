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

namespace spv {
MODULE_LOGGER

// testnet port
enum { DEFAULT_PORT = 18332 };

Addr::Addr(const addrinfo *ai) : af_(ai->ai_family) {
  assert(ai->ai_family == ai->ai_addr->sa_family);

  char buf[INET6_ADDRSTRLEN];
  memset(buf, 0, sizeof buf);

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
  uvw_addr_.port = DEFAULT_PORT;
  uvw_addr_.ip = buf;
}
}
