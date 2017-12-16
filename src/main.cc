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

#include "./client.h"
#include "./fs.h"
#include "./logging.h"
#include "./settings.h"
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
  int ret = -1;
  const spv::Settings& settings = spv::parse_settings(argc, argv, &ret);
  if (ret != -1) {
    return ret;
  }
  spv::FileLock lock;
  if (lock.lock(settings.lockfile)) {
    main_log->error("failed to acquire lock on lock file: {}",
                    settings.lockfile);
    return 1;
  }

  auto loop = uvw::Loop::getDefault();
  client.reset(new spv::Client(settings, loop));
  install_shutdown(SIGINT);
  install_shutdown(SIGTERM);
  client->run();

  loop->run();
  loop->close();
  return 0;
}
