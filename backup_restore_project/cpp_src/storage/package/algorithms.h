#pragma once
#include <string>

namespace pkg {

enum class PackAlg { HeaderPerFile = 1, TocAtEnd = 2 };
enum class CompressAlg { None = 0, RLE = 1 };
enum class EncryptAlg { None = 0, XOR = 1, RC4 = 2 };

inline PackAlg parsePack(const std::string& s) {
    if (s == "toc") return PackAlg::TocAtEnd;
    return PackAlg::HeaderPerFile; // default: header
}

inline CompressAlg parseCompress(const std::string& s) {
    if (s == "rle") return CompressAlg::RLE;
    return CompressAlg::None;
}

inline EncryptAlg parseEncrypt(const std::string& s) {
    if (s == "xor") return EncryptAlg::XOR;
    if (s == "rc4") return EncryptAlg::RC4;
    return EncryptAlg::None;
}

} // namespace pkg
