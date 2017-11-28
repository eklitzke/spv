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

#include <memory>

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#define EXTERN_LOGGER(name) extern std::shared_ptr<spdlog::logger> name##_log;

#define MODULE_LOGGER                          \
  static std::shared_ptr<spdlog::logger> log = \
      spdlog::stdout_color_st(__FILE__);
