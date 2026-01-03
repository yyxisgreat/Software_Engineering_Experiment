#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace pkg {

// RC4 流加密/解密（同一个函数）
// salt: 每个包随机生成，用于增强 key
std::vector<uint8_t> rc4_crypt(const std::vector<uint8_t>& in,
                              const std::string& password,
                              const std::vector<uint8_t>& salt);

} // namespace pkg
