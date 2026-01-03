#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace backuprestore {

/**
 * @brief 打包/解包存储接口（预留）
 * 
 * 数据格式说明：
 * - 打包格式：可以将多个文件打包成单个文件（如tar格式）
 * - 索引分离：元数据索引与文件数据可以分离存储
 * - 版本控制：支持增量备份和版本管理
 * 
 * TODO: 实现打包功能
 */
class PackStore {
public:
    virtual ~PackStore() = default;

    /**
     * @brief 打包文件列表到单个文件
     * @param files 要打包的文件路径列表
     * @param output_path 输出打包文件路径
     * @return 是否成功
     */
    virtual bool pack(const std::vector<std::filesystem::path>& files,
                      const std::filesystem::path& output_path) = 0;

    /**
     * @brief 解包文件到指定目录
     * @param pack_path 打包文件路径
     * @param output_dir 输出目录
     * @return 是否成功
     */
    virtual bool unpack(const std::filesystem::path& pack_path,
                        const std::filesystem::path& output_dir) = 0;

    /**
     * @brief 列出打包文件中的内容
     */
    virtual std::vector<std::filesystem::path> list(const std::filesystem::path& pack_path) = 0;
};

/**
 * @brief PackStore的简单实现（当前使用文件系统直接存储）
 * 预留接口，未来可以实现tar等打包格式
 */
class SimplePackStore : public PackStore {
public:
    bool pack(const std::vector<std::filesystem::path>& files,
              const std::filesystem::path& output_path) override;

    bool unpack(const std::filesystem::path& pack_path,
                const std::filesystem::path& output_dir) override;

    std::vector<std::filesystem::path> list(const std::filesystem::path& pack_path) override;
};

} // namespace backuprestore


