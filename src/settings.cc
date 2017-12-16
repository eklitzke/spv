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
int parse_settings(int argc, char** argv, Settings* settings) {
  cxxopts::Options options("spv", "A simple Bitcoin client.");
  auto g = options.add_options();
  g("d,debug", "Enable debugging");
  g("c,connections", "Max connections to make",
    cxxopts::value<std::size_t>()->default_value("8"));
  g("h,help", "Print help information");
  g("v,version", "Print version information");
  g("data-dir", "Path to the SPV database",
    cxxopts::value<std::string>()->default_value(".spv"));
  g("delete-data", "Delete the SPV data directory");

  try {
    auto args = options.parse(argc, argv);
    if (args.count("help")) {
      std::cout << options.help();
      return 0;
    }
    if (args.count("debug")) {
      spdlog::set_level(spdlog::level::debug);
      settings->debug = true;
    }
    if (args.count("delete-data")) {
      spv::recursive_delete(".spv");
    }
    if (args.count("version")) {
      std::cout << PACKAGE_STRING << std::endl;
      return 0;
    }
    settings->max_connections = args["connections"].as<std::size_t>();
    settings->datadir = args["data-dir"].as<std::string>();
  } catch (const cxxopts::option_not_exists_exception& exc) {
    std::cerr << exc.what() << "\n\n" << options.help();
    return 1;
  }
  return -1;
}
}  // namespace spv
