#include "filters/name_filter.h"

namespace backuprestore {

void NameFilter::addContains(const std::string& keyword) {
    keywords_.push_back(keyword);
}

bool NameFilter::shouldInclude(const std::filesystem::path& path) const {
    if (keywords_.empty()) {
        return true;
    }
    std::string filename = path.filename().string();
    for (const auto& kw : keywords_) {
        if (filename.find(kw) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace backuprestore