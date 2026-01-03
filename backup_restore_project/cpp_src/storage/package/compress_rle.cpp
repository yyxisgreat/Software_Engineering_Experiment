#include "compress_rle.h"
#include <stdexcept>

namespace pkg {

// 格式：[count(1字节)][byte(1字节)]...  count范围1..255
std::vector<uint8_t> rle_compress(const std::vector<uint8_t>& in) {
    std::vector<uint8_t> out;
    if (in.empty()) return out;

    size_t i = 0;
    while (i < in.size()) {
        uint8_t b = in[i];
        size_t j = i + 1;
        while (j < in.size() && in[j] == b && (j - i) < 255) ++j;
        uint8_t count = static_cast<uint8_t>(j - i);
        out.push_back(count);
        out.push_back(b);
        i = j;
    }
    return out;
}

std::vector<uint8_t> rle_decompress(const std::vector<uint8_t>& in) {
    std::vector<uint8_t> out;
    if (in.empty()) return out;
    if (in.size() % 2 != 0) throw std::runtime_error("RLE data corrupted");

    for (size_t i = 0; i < in.size(); i += 2) {
        uint8_t count = in[i];
        uint8_t b = in[i + 1];
        if (count == 0) throw std::runtime_error("RLE count=0 corrupted");
        out.insert(out.end(), count, b);
    }
    return out;
}

} // namespace pkg
