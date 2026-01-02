#include "encrypt_xor.h"
#include <cstdint>

namespace pkg {

// FNV-1a 32bit hash
static uint32_t fnv1a32(const std::string& s, const std::vector<uint8_t>& salt) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    for (uint8_t b : salt) { h ^= b; h *= 16777619u; }
    return h;
}

// xorshift32 生成伪随机字节流（教学用）
static uint8_t next_byte(uint32_t& x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return static_cast<uint8_t>(x & 0xFF);
}

std::vector<uint8_t> xor_crypt(const std::vector<uint8_t>& in,
                              const std::string& password,
                              const std::vector<uint8_t>& salt) {
    std::vector<uint8_t> out(in.size());
    uint32_t state = fnv1a32(password, salt);
    for (size_t i = 0; i < in.size(); ++i) {
        out[i] = in[i] ^ next_byte(state);
    }
    return out;
}

} // namespace pkg
