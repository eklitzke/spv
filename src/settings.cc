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

#include "./settings.h"

#include "cxxopts.hpp"

#include "./config.h"
#include "./fs.h"
#include "./logging.h"

namespace spv {
MODULE_LOGGER

static Settings settings_;
static bool did_parse = false;

const Settings& parse_settings(int argc, char** argv, int* ret) {
  assert(!did_parse);
  did_parse = true;

  cxxopts::Options options("spv", "A simple Bitcoin client.");
  auto g = options.add_options();
  g("d,debug", "Enable debugging");
  g("c,connections", "Max connections to make",
    cxxopts::value<std::size_t>()->default_value("8"));
  g("h,help", "Print help information");
  g("v,version", "Print version information");
  g("data-dir", "Path to the SPV database",
    cxxopts::value<std::string>()->default_value(".spv"));
  g("lock-file", "Path to the SPV lock file",
    cxxopts::value<std::string>()->default_value(".lock"));
  g("delete-data", "Delete the SPV data directory");

  g("protocol-version", "Protocol version to advertise",
    cxxopts::value<uint32_t>()->default_value(PROTOCOL_VERSION));
  g("protocol-port", "Port to use",
    cxxopts::value<uint16_t>()->default_value(PROTOCOL_PORT));
  g("protocol-user-agent", "User agent to advertise",
    cxxopts::value<std::string>()->default_value(USER_AGENT));

  try {
    auto args = options.parse(argc, argv);
    if (args.count("help")) {
      std::cout << options.help();
      *ret = 0;
      goto finish;
    }
    if (args.count("debug")) {
      spdlog::set_level(spdlog::level::debug);
      settings_.debug = true;
    }
    if (args.count("delete-data")) {
      spv::recursive_delete(".spv");
    }
    if (args.count("version")) {
      std::cout << PACKAGE_STRING << std::endl;
      *ret = 0;
      goto finish;
    }
    settings_.max_connections = args["connections"].as<std::size_t>();
    settings_.datadir = args["data-dir"].as<std::string>();
    settings_.lockfile = args["lock-file"].as<std::string>();
    settings_.version = args["protocol-version"].as<uint32_t>();
    settings_.port = args["protocol-port"].as<uint16_t>();
    settings_.user_agent = args["protocol-user-agent"].as<std::string>();
  } catch (const cxxopts::option_not_exists_exception& exc) {
    std::cerr << exc.what() << "\n\n" << options.help();
    *ret = 1;
    goto finish;
  }
finish:
  return settings_;
}

const Settings& get_settings() {
  assert(did_parse);
  return settings_;
}
}  // namespace spv
