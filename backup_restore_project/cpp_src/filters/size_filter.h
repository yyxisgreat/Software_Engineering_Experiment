#pragma once

#include "filters/filter_base.h"
#include <optional>

namespace backuprestore {

/**
 * @brief 文件大小过滤器
 * 根据文件大小过滤备份文件
 */
class SizeFilter : public FilterBase {
public:
    /**
     * @brief 设置最小文件大小（字节）
     */
    void setMinSize(std::int64_t size) { min_size_ = size; }
    /**
     * @brief 设置最大文件大小（字节）
     */
    void setMaxSize(std::int64_t size) { max_size_ = size; }

    bool shouldInclude(const std::filesystem::path& path) const override;
    Type getType() const override { return Type::Size; }

private:
    std::optional<std::int64_t> min_size_;
    std::optional<std::int64_t> max_size_;
};

} // namespace backuprestore