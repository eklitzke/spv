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

#include <cstddef>
#include <string>

#include "./config.h"

namespace spv {

struct Settings {
  bool debug;
  size_t max_connections;
  std::string datadir;
  std::string lockfile;

  // protocol options
  uint32_t version;
  uint16_t port;
  std::string user_agent;

  Settings()
      : debug(false),
        max_connections(8),
        datadir(".spv"),
        lockfile(".lock"),
        version(0),
        port(0),
        user_agent(USER_AGENT) {}
};

// parse settings from the command line arguments
const Settings &parse_settings(int argc, char **argv, int *ret);

// get the global settings singleton
const Settings &get_settings();
}  // namespace spv
