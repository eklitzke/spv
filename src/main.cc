#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "./encoder.h"

namespace spv {

// copied from chainparams.cpp
static const std::vector<std::string> mainSeeds = {
    "seed.bitcoin.sipa.be",          "dnsseed.bluematt.me",
    "dnsseed.bitcoin.dashjr.org",    "seed.bitcoinstats.com",
    "seed.bitcoin.jonasschnelli.ch", "seed.btc.petertodd.org",
};

}  // namespace spv

int main(int argc, char **argv) { return 0; }
