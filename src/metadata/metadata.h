#pragma once

#include <string>
#include <filesystem>
#include <cstdint>
#include <ctime>

namespace backuprestore {

/**
 * @brief 文件元数据类
 * 支持 mode（权限）和 mtime（修改时间）
 * uid/gid 预留接口
 */
class Metadata {
public:
    std::uint32_t mode = 0;      // 文件权限模式
    std::time_t mtime = 0;       // 修改时间
    std::uint32_t uid = 0;       // 用户ID（预留）
    std::uint32_t gid = 0;       // 组ID（预留）
    bool is_symlink = false;     // 是否为符号链接
    std::string symlink_target;  // 符号链接目标（如果适用）

    /**
     * @brief 从文件系统读取元数据
     * @param path 文件路径
     * @return 是否成功
     */
    bool loadFromFile(const std::filesystem::path& path);

    /**
     * @brief 将元数据应用到文件
     * @param path 文件路径
     * @return 是否成功
     */
    bool applyToFile(const std::filesystem::path& path) const;

    /**
     * @brief 序列化为字符串（用于保存到备份仓库）
     * @return 序列化字符串
     */
    std::string serialize() const;

    /**
     * @brief 从字符串反序列化
     * @param data 序列化字符串
     * @return 是否成功
     */
    bool deserialize(const std::string& data);

private:
    /**
     * @brief 从stat结构获取元数据（Linux系统调用）
     */
    bool loadFromStat(const std::filesystem::path& path);
};

} // namespace backuprestore


