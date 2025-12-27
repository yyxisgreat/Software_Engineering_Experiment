#include "filters/user_filter.h"
#include <sys/stat.h>

namespace backuprestore {

bool UserFilter::shouldInclude(const std::filesystem::path& path) const {
    if (!uid_ && !gid_) {
        return true;
    }
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        // 无法获取状态，默认包含
        return true;
    }
    if (uid_ && st.st_uid != *uid_) {
        return false;
    }
    if (gid_ && st.st_gid != *gid_) {
        return false;
    }
    return true;
}

} // namespace backuprestore