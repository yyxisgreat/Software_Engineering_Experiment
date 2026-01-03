#pragma once

#include <string>
#include <filesystem>
#include <map>
#include <vector>
#include "metadata/metadata.h"

namespace backuprestore {

/**
 * @brief 备份仓库类
 * 管理备份数据的存储结构和索引
 */
class Repository {
public:
    /**
     * @brief 构造函数
     * @param repo_path 备份仓库根目录
     */
    explicit Repository(const std::filesystem::path& repo_path);

    /**
     * @brief 初始化仓库（创建必要的目录结构）
     * @return 是否成功
     */
    bool initialize();

    /**
     * @brief 保存文件到仓库
     * @param source_path 源文件路径
     * @param relative_path 相对路径（在备份中的路径）
     * @param metadata 文件元数据
     * @return 是否成功
     */
    bool storeFile(const std::filesystem::path& source_path,
                   const std::filesystem::path& relative_path,
                   const Metadata& metadata);

    /**
     * @brief 从仓库恢复文件
     * @param relative_path 相对路径
     * @param target_path 目标路径
     * @param metadata 输出元数据
     * @return 是否成功
     */
    bool restoreFile(const std::filesystem::path& relative_path,
                     const std::filesystem::path& target_path,
                     Metadata& metadata);

    /**
     * @brief 保存索引（文件列表和元数据）
     * @return 是否成功
     */
    bool saveIndex();

    /**
     * @brief 加载索引
     * @return 是否成功
     */
    bool loadIndex();

    /**
     * @brief 获取仓库中的所有文件列表
     */
    std::vector<std::filesystem::path> listFiles() const;

    /**
     * @brief 获取文件的元数据
     */
    bool getMetadata(const std::filesystem::path& relative_path, Metadata& metadata) const;

private:
    std::filesystem::path repo_path_;
    std::filesystem::path data_dir_;   // 数据目录
    std::filesystem::path index_file_; // 索引文件
    
    // 索引：相对路径 -> 元数据
    std::map<std::filesystem::path, Metadata> index_;

    /**
     * @brief 获取文件在仓库中的存储路径
     */
    std::filesystem::path getStoragePath(const std::filesystem::path& relative_path) const;
};

} // namespace backuprestore


