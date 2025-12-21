#include "storage/pack_store.h"
#include <iostream>

namespace backuprestore {

bool SimplePackStore::pack(const std::vector<std::filesystem::path>& files,
                           const std::filesystem::path& output_path) {
    // TODO: 实现打包功能
    // 当前实现：仅作为占位符
    std::cerr << "PackStore::pack() 尚未实现" << std::endl;
    return false;
}

bool SimplePackStore::unpack(const std::filesystem::path& pack_path,
                             const std::filesystem::path& output_dir) {
    // TODO: 实现解包功能
    std::cerr << "PackStore::unpack() 尚未实现" << std::endl;
    return false;
}

std::vector<std::filesystem::path> SimplePackStore::list(const std::filesystem::path& pack_path) {
    // TODO: 实现列表功能
    return {};
}

} // namespace backuprestore


