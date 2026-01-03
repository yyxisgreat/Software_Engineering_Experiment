#pragma once

#include "filters/filter_base.h"
#include <vector>
#include <string>

namespace backuprestore {

/**
 * @brief 文件名过滤器
 * 包含指定关键字的文件才会被包含
 */
class NameFilter : public FilterBase {
public:
    /**
     * @brief 添加一个包含关键字
     * @param keyword 包含的关键字
     */
    void addContains(const std::string& keyword);

    bool shouldInclude(const std::filesystem::path& path) const override;

    Type getType() const override { return Type::Name; }

private:
    std::vector<std::string> keywords_;
};

} // namespace backuprestore