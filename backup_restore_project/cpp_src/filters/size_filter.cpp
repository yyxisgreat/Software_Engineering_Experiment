#include "filters/size_filter.h"
#include "core/file_utils.h"

namespace backuprestore {

bool SizeFilter::shouldInclude(const std::filesystem::path& path) const {
    // 只有常规文件和符号链接有实际大小
    if (!min_size_ && !max_size_) {
        return true;
    }
    // 获取文件大小；符号链接大小无意义，这里直接返回 true
    if (std::filesystem::is_symlink(path)) {
        return true;
    }
    if (!std::filesystem::is_regular_file(path)) {
        // 其他类型不基于大小过滤
        return true;
    }
    std::int64_t size = FileUtils::getFileSize(path);
    if (size < 0) {
        return true;
    }
    if (min_size_ && size < *min_size_) {
        return false;
    }
    if (max_size_ && size > *max_size_) {
        return false;
    }
    return true;
}

} // namespace backuprestore