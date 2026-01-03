#pragma once

#include "filters/filter_base.h"
#include <memory>
#include <vector>

namespace backuprestore {

/**
 * @brief 过滤器链，用于组合多个过滤器
 * 所有过滤器返回true时才包含该文件
 */
class FilterChain : public FilterBase {
public:
    /**
     * @brief 向链中添加一个过滤器
     * @param filter 智能指针指向的过滤器，会被转移所有权
     */
    void addFilter(std::unique_ptr<FilterBase> filter);

    bool shouldInclude(const std::filesystem::path& path) const override;

    // 过滤器链本身不对应某一具体类型
    Type getType() const override { return Type::Path; }

private:
    std::vector<std::unique_ptr<FilterBase>> filters_;
};

} // namespace backuprestore