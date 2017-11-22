#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "./client.h"
#include "./encoder.h"

namespace {
// copied from stack overflow
std::string string_to_hex(const std::string& input) {
  static const char* const lut = "0123456789abcdef";
  size_t len = input.length();

  std::string output;
  output.reserve(2 * len);
  for (size_t i = 0; i < len; ++i) {
    const unsigned char c = input[i];
    output.push_back(lut[c >> 4]);
    output.push_back(lut[c & 15]);
  }
  return output;
}

// copied from chainparams.cpp
static const std::vector<std::string> mainSeeds = {
    "seed.bitcoin.sipa.be",          "dnsseed.bluematt.me",
    "dnsseed.bitcoin.dashjr.org",    "seed.bitcoinstats.com",
    "seed.bitcoin.jonasschnelli.ch", "seed.btc.petertodd.org",
};
}  // namespace

int main(int argc, char** argv) {
  spv::NetAddr addr;
  spv::Encoder enc("version");
  spv::Client client;
  client.send_version(&enc, 100, addr);
  std::cout << string_to_hex(enc.serialize()) << "\n";
  return 0;
}
