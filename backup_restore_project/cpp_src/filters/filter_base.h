#pragma once

#include <filesystem>
#include <string>

namespace backuprestore {

/**
 * @brief 过滤器基类
 * 用于自定义备份时的文件过滤
 */
class FilterBase {
public:
    virtual ~FilterBase() = default;

    /**
     * @brief 判断文件是否应该被包含
     * @param path 文件路径
     * @return true表示包含，false表示排除
     */
    virtual bool shouldInclude(const std::filesystem::path& path) const = 0;

    /**
     * @brief 过滤器类型
     */
    enum class Type {
        Path,           // 路径过滤器（已实现）
        FileType,       // 文件类型过滤（预留）
        Name,           // 文件名过滤（预留）
        Time,           // 时间过滤（预留）
        Size,           // 大小过滤（预留）
        User            // 用户过滤（预留）
    };

    virtual Type getType() const = 0;
};

} // namespace backuprestore


