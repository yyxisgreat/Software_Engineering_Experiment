#include "filters/path_filter.h"
#include <algorithm>
#include <iostream>

namespace backuprestore {

void PathFilter::addInclude(const std::string& pattern) {
    include_patterns_.push_back(pattern);
}

void PathFilter::addExclude(const std::string& pattern) {
    exclude_patterns_.push_back(pattern);
}

void PathFilter::clear() {
    include_patterns_.clear();
    exclude_patterns_.clear();
}

bool PathFilter::shouldInclude(const std::filesystem::path& path) const {
    // 如果没有任何规则，默认包含
    if (include_patterns_.empty() && exclude_patterns_.empty()) {
        return true;
    }

    std::string path_str = path.string();

    // 先检查排除规则
    for (const auto& exclude : exclude_patterns_) {
        if (matchesPattern(path, exclude)) {
            return false;
        }
    }

    // 如果有包含规则，检查是否匹配
    if (!include_patterns_.empty()) {
        for (const auto& include : include_patterns_) {
            if (matchesPattern(path, include)) {
                return true;
            }
        }
        return false;  // 不匹配任何包含规则
    }

    return true;  // 没有排除规则且没有包含规则（或已通过排除检查）
}

bool PathFilter::matchesPattern(const std::filesystem::path& path, 
                                const std::string& pattern) const {
    std::string path_str = path.string();
    
    // 简单前缀匹配
    if (pattern.back() == '/') {
        // 目录模式：检查是否为该目录下的文件
        std::string dir_pattern = pattern.substr(0, pattern.length() - 1);
        return path_str.find(dir_pattern) == 0;
    } else {
        // 精确匹配或包含匹配
        return path_str == pattern || path_str.find(pattern) != std::string::npos;
    }
}

} // namespace backuprestore


