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

#include <signal.h>

#include <cstring>
#include <string>
#include <vector>

#include "./cxxopts.hpp"

#include "./client.h"
#include "./config.h"
#include "./fs.h"
#include "./logging.h"
#include "./util.h"
#include "./uvw.h"

namespace {
DECLARE_LOGGER(main_log)
std::unique_ptr<spv::Client> client;
}

static void shutdown(void) {
  client->shutdown();
  auto loop = uvw::Loop::getDefault();
  loop->walk([](uvw::BaseHandle& h) {
    if (h.closing()) {
      main_log->debug("loop handle {} already closing", (void*)&h);
    } else {
      main_log->info("closing pending handle {}", (void*)&h);
      h.close();
    }
  });
}

static void install_shutdown(int signum) {
  auto loop = uvw::Loop::getDefault();
  auto handle = loop->resource<uvw::SignalHandle>();
  handle->on<uvw::SignalEvent>([=](const auto&, auto& h) {
    main_log->info("received signal {}", signum);
    shutdown();
    h.close();
  });
  handle->start(signum);
}

int main(int argc, char** argv) {
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

  std::string data_dir;
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
    if (args.count("delete-data")) {
      spv::recursive_delete(".spv");
    }
    if (args.count("version")) {
      std::cout << PACKAGE_STRING << std::endl;
      return 0;
    }
    connections = args["connections"].as<std::size_t>();
    data_dir = args["data-dir"].as<std::string>();
  } catch (const cxxopts::option_not_exists_exception& exc) {
    std::cerr << exc.what() << "\n\n" << options.help();
    return 1;
  }

  auto loop = uvw::Loop::getDefault();
  client.reset(new spv::Client(data_dir, loop, connections));
  install_shutdown(SIGINT);
  install_shutdown(SIGTERM);
  client->run();

  loop->run();
  loop->close();
  return 0;
}
