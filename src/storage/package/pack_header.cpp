#include "pack_header.h"
#include "binary_io.h"

namespace pkg {

// 写入：fileCount + [path + origSize + storedSize + data]...
void pack_header_write(std::ostream& os, const std::vector<Entry>& entries) {
    write_le<uint32_t>(os, static_cast<uint32_t>(entries.size()));
    for (const auto& e : entries) {
        write_string(os, e.relPath);
        write_le<uint64_t>(os, e.originalSize);
        write_le<uint64_t>(os, static_cast<uint64_t>(e.payload.size()));
        write_bytes(os, e.payload);
    }
}

std::vector<Entry> pack_header_read(std::istream& is) {
    uint32_t n = read_le<uint32_t>(is);
    std::vector<Entry> entries;
    entries.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        Entry e;
        e.relPath = read_string(is);
        e.originalSize = read_le<uint64_t>(is);
        uint64_t stored = read_le<uint64_t>(is);
        e.payload = read_bytes(is, static_cast<size_t>(stored));
        entries.push_back(std::move(e));
    }
    return entries;
}

} // namespace pkg
