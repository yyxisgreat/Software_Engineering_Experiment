#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <istream>
#include <ostream>

namespace pkg {

// TOC 条目：路径 + 原始大小 + offset + storedSize
struct TocItem {
    std::string relPath;
    uint64_t originalSize = 0;
    uint64_t offset = 0;
    uint64_t storedSize = 0;
};

// 算法2：先写所有数据 blob，末尾写 TOC + tocOffset
void pack_toc_write(std::ostream& os,
                    const std::vector<TocItem>& toc,
                    const std::vector<std::vector<uint8_t>>& blobs);

void pack_toc_read(std::istream& is,
                   std::vector<TocItem>& tocOut,
                   std::vector<std::vector<uint8_t>>& blobsOut);

} // namespace pkg
