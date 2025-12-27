#pragma once

#include "filters/filter_base.h"
#include <optional>

namespace backuprestore {

/**
 * @brief 用户过滤器
 * 根据文件所属用户/组ID过滤
 */
class UserFilter : public FilterBase {
public:
    /**
     * @brief 设置要求的 uid（用户ID）
     */
    void setUid(std::uint32_t id) { uid_ = id; }
    /**
     * @brief 设置要求的 gid（组ID）
     */
    void setGid(std::uint32_t id) { gid_ = id; }

    bool shouldInclude(const std::filesystem::path& path) const override;
    Type getType() const override { return Type::User; }

private:
    std::optional<std::uint32_t> uid_;
    std::optional<std::uint32_t> gid_;
};

} // namespace backuprestore