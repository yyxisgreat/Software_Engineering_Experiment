#include "core/restore.h"
#include "core/file_utils.h"
#include "metadata/metadata.h"
#include <iostream>

namespace backuprestore {

Restore::Restore(std::shared_ptr<Repository> repo) : repo_(repo) {
}

bool Restore::execute(const std::filesystem::path& target_root) {
    // 加载索引
    if (!repo_->loadIndex()) {
        std::cerr << "加载索引失败" << std::endl;
        return false;
    }

    // 获取所有文件列表
    auto files = repo_->listFiles();
    std::cout << "仓库中有 " << files.size() << " 个文件" << std::endl;

    restore_count_ = 0;
    failed_count_ = 0;

    // 还原每个文件
    for (const auto& relative_path : files) {
        if (restoreFile(relative_path, target_root)) {
            restore_count_++;
        } else {
            failed_count_++;
        }
    }

    std::cout << "还原完成: " << restore_count_ << " 个文件已还原, " 
              << failed_count_ << " 个文件失败" << std::endl;

    return failed_count_ == 0;
}

bool Restore::restoreFile(const std::filesystem::path& relative_path,
                          const std::filesystem::path& target_root) {
    try {
        // 计算目标路径
        auto target_path = target_root / relative_path;

        // 从仓库恢复文件
        Metadata metadata;
        if (!repo_->restoreFile(relative_path, target_path, metadata)) {
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "还原文件失败: " << relative_path << " - " << e.what() << std::endl;
        return false;
    }
}

} // namespace backuprestore


