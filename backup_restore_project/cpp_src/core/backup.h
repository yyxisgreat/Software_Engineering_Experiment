#pragma once

#include <filesystem>
#include <string>
#include <memory>
#include "core/repository.h"
#include "filters/filter_base.h"

namespace backuprestore {

/**
 * @brief 备份操作类
 * 负责将目录树备份到仓库
 */
class Backup {
public:
    /**
     * @brief 构造函数
     * @param repo 备份仓库
     */
    explicit Backup(std::shared_ptr<Repository> repo);

    /**
     * @brief 执行备份
     * @param source_root 源目录根路径
     * @param filter 过滤器（可选，nullptr表示不过滤）
     * @return 是否成功
     */
    bool execute(const std::filesystem::path& source_root,
                 const FilterBase* filter = nullptr);

    /**
     * @brief 获取备份的文件数量
     */
    std::size_t getBackupCount() const { return backup_count_; }

    /**
     * @brief 获取跳过的文件数量
     */
    std::size_t getSkippedCount() const { return skipped_count_; }

private:
    std::shared_ptr<Repository> repo_;
    std::size_t backup_count_ = 0;
    std::size_t skipped_count_ = 0;

    /**
     * @brief 备份单个文件
     */
    bool backupFile(const std::filesystem::path& source_path,
                    const std::filesystem::path& source_root);
};

} // namespace backuprestore


