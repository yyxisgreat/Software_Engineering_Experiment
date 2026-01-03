#include "filters/time_filter.h"
#include <sys/stat.h>

namespace backuprestore {

bool TimeFilter::shouldInclude(const std::filesystem::path& path) const {
    // 如果没有设置任何时间限制，则包含
    if (!after_ && !before_) {
        return true;
    }
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        // 无法获取状态，默认包含
        return true;
    }
    std::time_t mtime = st.st_mtime;
    if (after_ && mtime < *after_) {
        return false;
    }
    if (before_ && mtime > *before_) {
        return false;
    }
    return true;
}

} // namespace backuprestore