#pragma once

#include "filters/filter_base.h"
#include <vector>
#include <string>

namespace backuprestore {

/**
 * @brief 路径过滤器
 * 支持 include/exclude 路径模式
 */
class PathFilter : public FilterBase {
public:
    /**
     * @brief 添加包含路径
     */
    void addInclude(const std::string& pattern);

    /**
     * @brief 添加排除路径
     */
    void addExclude(const std::string& pattern);

    /**
     * @brief 清除所有规则
     */
    void clear();

    bool shouldInclude(const std::filesystem::path& path) const override;
    Type getType() const override { return Type::Path; }

private:
    std::vector<std::string> include_patterns_;
    std::vector<std::string> exclude_patterns_;

    /**
     * @brief 检查路径是否匹配模式（简单前缀匹配）
     * TODO: 支持更复杂的模式（glob/regex）
     */
    bool matchesPattern(const std::filesystem::path& path, 
                        const std::string& pattern) const;
};

} // namespace backuprestore


