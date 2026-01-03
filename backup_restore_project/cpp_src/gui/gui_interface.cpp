#include "gui/gui_interface.h"
#include "core/repository.h"
#include "core/backup.h"
#include "core/restore.h"
#include "core/file_utils.h"
#include "filters/path_filter.h"
#include "metadata/metadata.h"
#include "metadata/filesystem.h"
#include <filesystem>
#include <algorithm>

namespace backuprestore {

bool GuiOperations::backupWithProgress(
    const std::filesystem::path& source_root,
    const std::filesystem::path& repo_path,
    const std::vector<std::filesystem::path>& include_paths,
    const std::vector<std::filesystem::path>& exclude_paths,
    ProgressCallback* callback) {
    
    // 验证源目录
    if (!std::filesystem::exists(source_root)) {
        if (callback) {
            callback->onFileError(source_root, "源目录不存在");
        }
        return false;
    }

    // 创建仓库
    auto repo = std::make_shared<Repository>(repo_path);
    if (!repo->initialize()) {
        if (callback) {
            callback->onFileError(repo_path, "初始化仓库失败");
        }
        return false;
    }

    // 获取所有文件
    std::vector<std::filesystem::path> files;
    FileUtils::getFilesRecursive(source_root, files);

    // 创建过滤器
    std::unique_ptr<PathFilter> filter;
    bool has_filter = !include_paths.empty() || !exclude_paths.empty();
    if (has_filter) {
        filter = std::make_unique<PathFilter>();
        for (const auto& path : include_paths) {
            filter->addInclude(path);
        }
        for (const auto& path : exclude_paths) {
            filter->addExclude(path);
        }
    }

    // 通知开始
    if (callback) {
        callback->onStart(files.size(), "备份");
    }

    std::size_t success_count = 0;
    std::size_t failed_count = 0;
    std::size_t skipped_count = 0;

    // 处理每个文件
    for (std::size_t i = 0; i < files.size(); ++i) {
        const auto& file_path = files[i];

        // 检查是否取消
        if (callback && callback->shouldCancel()) {
            if (callback) {
                callback->onComplete(success_count, failed_count, skipped_count, false);
            }
            return false;
        }

        // 更新进度
        if (callback) {
            double percentage = (i + 1) * 100.0 / files.size();
            callback->onProgress(file_path, i + 1, files.size(), percentage);
        }

        // 应用过滤器
        if (filter && !filter->shouldInclude(file_path)) {
            skipped_count++;
            if (callback) {
                callback->onFileSkipped(file_path, "被过滤器排除");
            }
            continue;
        }

        // 检查文件类型
        auto file_type = FilesystemUtils::getFileType(file_path);
        if (!FilesystemUtils::isBackupSupported(file_type)) {
            skipped_count++;
            if (callback) {
                callback->onFileSkipped(file_path, "不支持的文件类型");
            }
            continue;
        }

        // 备份文件
        try {
            auto relative_path = FileUtils::getRelativePath(source_root, file_path);
            Metadata metadata;
            if (!metadata.loadFromFile(file_path)) {
                failed_count++;
                if (callback) {
                    callback->onFileError(file_path, "读取元数据失败");
                }
                continue;
            }

            if (repo->storeFile(file_path, relative_path, metadata)) {
                success_count++;
                if (callback) {
                    callback->onFileSuccess(file_path);
                }
            } else {
                failed_count++;
                if (callback) {
                    callback->onFileError(file_path, "存储到仓库失败");
                }
            }
        } catch (const std::exception& e) {
            failed_count++;
            if (callback) {
                callback->onFileError(file_path, std::string("异常: ") + e.what());
            }
        }
    }

    // 保存索引
    if (!repo->saveIndex()) {
        if (callback) {
            callback->onFileError(repo_path, "保存索引失败");
        }
        if (callback) {
            callback->onComplete(success_count, failed_count, skipped_count, false);
        }
        return false;
    }

    // 通知完成
    bool overall_success = failed_count == 0;
    if (callback) {
        callback->onComplete(success_count, failed_count, skipped_count, overall_success);
    }

    return overall_success;
}

bool GuiOperations::restoreWithProgress(
    const std::filesystem::path& repo_path,
    const std::filesystem::path& target_root,
    ProgressCallback* callback) {
    
    // 创建仓库
    auto repo = std::make_shared<Repository>(repo_path);
    if (!repo->loadIndex()) {
        if (callback) {
            callback->onFileError(repo_path, "加载仓库索引失败");
        }
        return false;
    }

    // 获取文件列表
    auto files = repo->listFiles();

    // 通知开始
    if (callback) {
        callback->onStart(files.size(), "还原");
    }

    std::size_t success_count = 0;
    std::size_t failed_count = 0;
    std::size_t skipped_count = 0;

    // 还原每个文件
    for (std::size_t i = 0; i < files.size(); ++i) {
        const auto& relative_path = files[i];

        // 检查是否取消
        if (callback && callback->shouldCancel()) {
            if (callback) {
                callback->onComplete(success_count, failed_count, skipped_count, false);
            }
            return false;
        }

        // 更新进度
        if (callback) {
            double percentage = (i + 1) * 100.0 / files.size();
            callback->onProgress(relative_path, i + 1, files.size(), percentage);
        }

        // 还原文件
        try {
            auto target_path = target_root / relative_path;
            Metadata metadata;
            if (repo->restoreFile(relative_path, target_path, metadata)) {
                success_count++;
                if (callback) {
                    callback->onFileSuccess(relative_path);
                }
            } else {
                failed_count++;
                if (callback) {
                    callback->onFileError(relative_path, "还原文件失败");
                }
            }
        } catch (const std::exception& e) {
            failed_count++;
            if (callback) {
                callback->onFileError(relative_path, std::string("异常: ") + e.what());
            }
        }
    }

    // 通知完成
    bool overall_success = failed_count == 0;
    if (callback) {
        callback->onComplete(success_count, failed_count, skipped_count, overall_success);
    }

    return overall_success;
}

std::vector<std::filesystem::path> GuiOperations::listBackupFiles(
    const std::filesystem::path& repo_path) {
    
    auto repo = std::make_shared<Repository>(repo_path);
    if (!repo->loadIndex()) {
        return {};
    }
    return repo->listFiles();
}

bool GuiOperations::validateRepository(const std::filesystem::path& repo_path) {
    // 检查仓库目录是否存在
    if (!std::filesystem::exists(repo_path)) {
        return false;
    }

    // 检查必要的目录和文件
    auto data_dir = repo_path / "data";
    auto index_file = repo_path / "index.txt";

    if (!std::filesystem::exists(data_dir) || !std::filesystem::is_directory(data_dir)) {
        return false;
    }

    if (!std::filesystem::exists(index_file) || !std::filesystem::is_regular_file(index_file)) {
        return false;
    }

    // 尝试加载索引
    auto repo = std::make_shared<Repository>(repo_path);
    return repo->loadIndex();
}

} // namespace backuprestore

