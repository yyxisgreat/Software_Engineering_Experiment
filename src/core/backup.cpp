#include "core/backup.h"
#include "core/file_utils.h"
#include "metadata/metadata.h"
#include "metadata/filesystem.h"
#include <iostream>

namespace backuprestore {

Backup::Backup(std::shared_ptr<Repository> repo) : repo_(repo) {
}

bool Backup::execute(const std::filesystem::path& source_root,
                     const FilterBase* filter) {
    if (!std::filesystem::exists(source_root)) {
        std::cerr << "源目录不存在: " << source_root << std::endl;
        return false;
    }

    backup_count_ = 0;
    skipped_count_ = 0;

    // 获取所有文件
    std::vector<std::filesystem::path> files;
    FileUtils::getFilesRecursive(source_root, files);

    std::cout << "找到 " << files.size() << " 个文件" << std::endl;

    // 备份每个文件
    for (const auto& file_path : files) {
        // 应用过滤器
        if (filter && !filter->shouldInclude(file_path)) {
            skipped_count_++;
            continue;
        }

        // 检查文件类型是否支持
        auto file_type = FilesystemUtils::getFileType(file_path);
        if (!FilesystemUtils::isBackupSupported(file_type)) {
            skipped_count_++;
            continue;
        }

        if (backupFile(file_path, source_root)) {
            backup_count_++;
        } else {
            skipped_count_++;
        }
    }

    // 保存索引
    if (!repo_->saveIndex()) {
        std::cerr << "保存索引失败" << std::endl;
        return false;
    }

    std::cout << "备份完成: " << backup_count_ << " 个文件已备份, " 
              << skipped_count_ << " 个文件已跳过" << std::endl;

    return true;
}

bool Backup::backupFile(const std::filesystem::path& source_path,
                        const std::filesystem::path& source_root) {
    try {
        // 计算相对路径
        auto relative_path = FileUtils::getRelativePath(source_root, source_path);
        
        // 读取元数据
        Metadata metadata;
        if (!metadata.loadFromFile(source_path)) {
            std::cerr << "读取元数据失败: " << source_path << std::endl;
            return false;
        }

        // 存储到仓库
        if (!repo_->storeFile(source_path, relative_path, metadata)) {
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "备份文件失败: " << source_path << " - " << e.what() << std::endl;
        return false;
    }
}

} // namespace backuprestore


