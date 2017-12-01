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

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "./cxxopts.hpp"

#include "./client.h"
#include "./config.h"
#include "./logging.h"
#include "./util.h"
#include "./uvw.h"

static const char usage_str[] = "Usage: spv [-h|--help] [-v|--version]\n";

int main(int argc, char** argv) {
  cxxopts::Options options("spv", "A simple SPV client.");
  options.add_options()("d,debug", "Enable debugging")(
      "c,connections", "Max connections to make",
      cxxopts::value<std::size_t>()->default_value("4"))(
      "h,help", "Print help information")("v,version",
                                          "Print version information");

  std::size_t connections;
  try {
    auto args = options.parse(argc, argv);
    if (args.count("help")) {
      std::cout << options.help();
      return 0;
    }
    if (args.count("debug")) {
      spdlog::set_level(spdlog::level::debug);
    }
    connections = args["connections"].as<std::size_t>();
  } catch (const cxxopts::option_not_exists_exception& exc) {
    std::cerr << exc.what() << "\n\n" << options.help();
    return 1;
  }

  auto loop = uvw::Loop::getDefault();
  spv::Client client(*loop, connections);
  client.run();

  loop->run();
  loop->close();
  return 0;
}
