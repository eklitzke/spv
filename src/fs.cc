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

#include "./fs.h"

#include <fts.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "./logging.h"

namespace spv {
MODULE_LOGGER

// Based on https://stackoverflow.com/a/27808574/7897084
int recursive_delete(const std::string &dirname) {
  char *files[] = {const_cast<char *>(dirname.c_str()), nullptr};
  FTS *ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, nullptr);
  if (ftsp == nullptr) {
    log->error("fts_open failed: {}", strerror(errno));
    return 1;
  }

  FTSENT *curr;
  while ((curr = fts_read(ftsp))) {
    switch (curr->fts_info) {
      case FTS_NS:
      case FTS_DNR:
      case FTS_ERR:
        log->error("{}: fts_read error: {}", curr->fts_accpath,
                   strerror(curr->fts_errno));
        break;

      case FTS_DC:
      case FTS_DOT:
      case FTS_NSOK:
        // Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
        // passed to fts_open()
        break;

      case FTS_D:
        // Do nothing. Need depth-first search, so directories are deleted
        // in FTS_DP
        break;

      case FTS_DP:
      case FTS_F:
      case FTS_SL:
      case FTS_SLNONE:
      case FTS_DEFAULT:
        if (remove(curr->fts_accpath) < 0) {
          log->error("{}: Failed to remove: {}", curr->fts_path,
                     strerror(errno));
          fts_close(ftsp);
          return 1;
        }
        break;
    }
  }
  fts_close(ftsp);
  return 0;
}
}  // namespace spv
