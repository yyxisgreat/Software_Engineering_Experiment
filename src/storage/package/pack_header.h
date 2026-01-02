#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <istream>
#include <ostream>

namespace pkg {

struct Entry {
    std::string relPath;           // 相对路径（用 /）
    std::vector<uint8_t> payload;  // 已压缩/加密后的数据
    uint64_t originalSize = 0;
};

// 算法1：每个文件前写 header：path + originalSize + storedSize + data
void pack_header_write(std::ostream& os, const std::vector<Entry>& entries);
std::vector<Entry> pack_header_read(std::istream& is);

} // namespace pkg
