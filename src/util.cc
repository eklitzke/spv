#include "./util.h"

#include <limits>
#include <random>

namespace {
std::random_device rd;
std::mt19937_64 gen(rd());
std::uniform_int_distribution<uint64_t> dist(
    0, std::numeric_limits<uint64_t>::max());
}

namespace spv {
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

uint64_t rand64() { return dist(gen); }
}
