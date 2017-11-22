#pragma once

#include <cstdint>
#include <string>

namespace spv {
// convert a raw byte string to hex
std::string string_to_hex(const std::string& input);

// generate a random uint64_t value
uint64_t rand64();
}  // namespace spv
