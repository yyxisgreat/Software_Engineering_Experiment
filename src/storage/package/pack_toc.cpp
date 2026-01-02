#include "pack_toc.h"
#include "binary_io.h"
#include <stdexcept>

namespace pkg {

static const char TOC_MAGIC[4] = {'T','O','C','1'};

void pack_toc_write(std::ostream& os,
                    const std::vector<TocItem>& toc,
                    const std::vector<std::vector<uint8_t>>& blobs) {
    if (toc.size() != blobs.size()) throw std::runtime_error("toc/blobs size mismatch");

    std::vector<TocItem> local = toc;

    // 先写 blobs，并记录offset
    for (size_t i = 0; i < blobs.size(); ++i) {
        local[i].offset = static_cast<uint64_t>(os.tellp());
        local[i].storedSize = static_cast<uint64_t>(blobs[i].size());
        write_bytes(os, blobs[i]);
    }

    uint64_t tocOffset = static_cast<uint64_t>(os.tellp());

    // 写 TOC block
    os.write(TOC_MAGIC, 4);
    write_le<uint32_t>(os, static_cast<uint32_t>(local.size()));
    for (const auto& item : local) {
        write_string(os, item.relPath);
        write_le<uint64_t>(os, item.originalSize);
        write_le<uint64_t>(os, item.offset);
        write_le<uint64_t>(os, item.storedSize);
    }

    // 文件末尾写 tocOffset（方便反向读）
    write_le<uint64_t>(os, tocOffset);
}

void pack_toc_read(std::istream& is,
                   std::vector<TocItem>& tocOut,
                   std::vector<std::vector<uint8_t>>& blobsOut) {
    // 读最后8字节 tocOffset
    is.seekg(0, std::ios::end);
    auto endPos = is.tellg();
    if (endPos < 8) throw std::runtime_error("file too small");

    is.seekg(endPos - std::streamoff(8), std::ios::beg);
    uint64_t tocOffset = read_le<uint64_t>(is);

    // 跳到 tocOffset 读取 TOC
    is.seekg(static_cast<std::streamoff>(tocOffset), std::ios::beg);

    char magic[4];
    is.read(magic, 4);
    if (!is || std::string(magic, 4) != std::string(TOC_MAGIC, 4))
        throw std::runtime_error("TOC magic mismatch");

    uint32_t n = read_le<uint32_t>(is);

    tocOut.clear();
    tocOut.reserve(n);

    for (uint32_t i = 0; i < n; ++i) {
        TocItem item;
        item.relPath = read_string(is);
        item.originalSize = read_le<uint64_t>(is);
        item.offset = read_le<uint64_t>(is);
        item.storedSize = read_le<uint64_t>(is);
        tocOut.push_back(std::move(item));
    }

    // 根据 toc 读 blobs
    blobsOut.clear();
    blobsOut.reserve(n);

    for (const auto& item : tocOut) {
        is.seekg(static_cast<std::streamoff>(item.offset), std::ios::beg);
        blobsOut.push_back(read_bytes(is, static_cast<size_t>(item.storedSize)));
    }
}

} // namespace pkg
