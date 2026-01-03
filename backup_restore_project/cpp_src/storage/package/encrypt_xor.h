#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace pkg {

// XOR 伪随机流加密/解密（同一个函数）
// salt: 每个包随机生成，用于生成伪随机流
std::vector<uint8_t> xor_crypt(const std::vector<uint8_t>& in,
                              const std::string& password,
                              const std::vector<uint8_t>& salt);

} // namespace pkg
