#pragma once

#include "filters/filter_base.h"
#include <ctime>
#include <optional>

namespace backuprestore {

/**
 * @brief 时间过滤器
 * 过滤基于最后修改时间的文件
 */
class TimeFilter : public FilterBase {
public:
    /**
     * @brief 设置最小修改时间（包含）
     * @param t 时间戳
     */
    void setAfter(std::time_t t) { after_ = t; }

    /**
     * @brief 设置最大修改时间（包含）
     */
    void setBefore(std::time_t t) { before_ = t; }

    bool shouldInclude(const std::filesystem::path& path) const override;

    Type getType() const override { return Type::Time; }

private:
    std::optional<std::time_t> after_;
    std::optional<std::time_t> before_;
};

} // namespace backuprestore