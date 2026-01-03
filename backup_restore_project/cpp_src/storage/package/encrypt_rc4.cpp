#include "encrypt_rc4.h"
#include <array>

namespace pkg {

static std::vector<uint8_t> make_key(const std::string& password, const std::vector<uint8_t>& salt) {
    std::vector<uint8_t> key;
    key.reserve(password.size() + salt.size());
    for (unsigned char c : password) key.push_back(static_cast<uint8_t>(c));
    key.insert(key.end(), salt.begin(), salt.end());
    if (key.empty()) key.push_back(0);
    return key;
}

std::vector<uint8_t> rc4_crypt(const std::vector<uint8_t>& in,
                              const std::string& password,
                              const std::vector<uint8_t>& salt) {
    auto key = make_key(password, salt);

    std::array<uint8_t, 256> S{};
    for (int i = 0; i < 256; ++i) S[i] = static_cast<uint8_t>(i);

    // KSA
    int j = 0;
    for (int i = 0; i < 256; ++i) {
        j = (j + S[i] + key[i % key.size()]) & 0xFF;
        std::swap(S[i], S[j]);
    }

    // PRGA
    std::vector<uint8_t> out(in.size());
    int i = 0; j = 0;
    for (size_t k = 0; k < in.size(); ++k) {
        i = (i + 1) & 0xFF;
        j = (j + S[i]) & 0xFF;
        std::swap(S[i], S[j]);
        uint8_t rnd = S[(S[i] + S[j]) & 0xFF];
        out[k] = in[k] ^ rnd;
    }
    return out;
}

} // namespace pkg
