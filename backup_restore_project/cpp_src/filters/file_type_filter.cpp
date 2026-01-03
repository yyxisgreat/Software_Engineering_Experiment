#include "filters/file_type_filter.h"

namespace backuprestore {

void FileTypeFilter::addAllowed(FilesystemUtils::FileType type) {
    // 避免重复添加
    for (auto& t : allowed_) {
        if (t == type) return;
    }
    allowed_.push_back(type);
}

bool FileTypeFilter::shouldInclude(const std::filesystem::path& path) const {
    if (allowed_.empty()) {
        // 若未指定类型，则全部允许
        return true;
    }
    auto type = FilesystemUtils::getFileType(path);
    for (const auto& allowed : allowed_) {
        if (type == allowed) return true;
    }
    return false;
}

} // namespace backuprestore