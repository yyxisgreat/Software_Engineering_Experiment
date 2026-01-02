#pragma once
#include <cstdint>
#include <vector>

namespace pkg {

std::vector<uint8_t> rle_compress(const std::vector<uint8_t>& in);
std::vector<uint8_t> rle_decompress(const std::vector<uint8_t>& in);

} // namespace pkg
