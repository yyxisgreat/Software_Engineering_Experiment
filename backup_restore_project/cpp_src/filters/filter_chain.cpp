#include "filters/filter_chain.h"

namespace backuprestore {

void FilterChain::addFilter(std::unique_ptr<FilterBase> filter) {
    if (filter) {
        filters_.push_back(std::move(filter));
    }
}

bool FilterChain::shouldInclude(const std::filesystem::path& path) const {
    for (const auto& f : filters_) {
        if (f && !f->shouldInclude(path)) {
            return false;
        }
    }
    return true;
}

} // namespace backuprestore