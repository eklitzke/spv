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
uint64_t rand64() { return dist(gen); }
}
