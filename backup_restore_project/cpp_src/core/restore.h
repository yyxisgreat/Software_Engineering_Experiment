#pragma once

#include <filesystem>
#include <memory>
#include "core/repository.h"

namespace backuprestore {

/**
 * @brief 还原操作类
 * 负责从仓库恢复文件到目录树
 */
class Restore {
public:
    /**
     * @brief 构造函数
     * @param repo 备份仓库
     */
    explicit Restore(std::shared_ptr<Repository> repo);

    /**
     * @brief 执行还原
     * @param target_root 目标目录根路径
     * @return 是否成功
     */
    bool execute(const std::filesystem::path& target_root);

    /**
     * @brief 获取还原的文件数量
     */
    std::size_t getRestoreCount() const { return restore_count_; }

    /**
     * @brief 获取失败的文件数量
     */
    std::size_t getFailedCount() const { return failed_count_; }

private:
    std::shared_ptr<Repository> repo_;
    std::size_t restore_count_ = 0;
    std::size_t failed_count_ = 0;

    /**
     * @brief 还原单个文件
     */
    bool restoreFile(const std::filesystem::path& relative_path,
                     const std::filesystem::path& target_root);
};

} // namespace backuprestore


