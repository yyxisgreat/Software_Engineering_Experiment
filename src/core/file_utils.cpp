#include "core/file_utils.h"
#include <fstream>
#include <iostream>
#include <cstdint>

namespace backuprestore {

void FileUtils::getFilesRecursive(const std::filesystem::path& root, 
                                   std::vector<std::filesystem::path>& files) {
    if (!std::filesystem::exists(root)) {
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        // 收集除目录外的所有文件类型。
        // 其他类型（FIFO、设备等）通过 FilesystemUtils 进一步判断是否支持
        if (!entry.is_directory()) {
            files.push_back(entry.path());
        }
    }
}

bool FileUtils::createDirectories(const std::filesystem::path& path) {
    try {
        std::filesystem::create_directories(path);
        return std::filesystem::exists(path);
    } catch (const std::exception& e) {
        std::cerr << "创建目录失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

bool FileUtils::copyFile(const std::filesystem::path& from, 
                         const std::filesystem::path& to) {
    try {
        // 确保目标目录存在
        auto parent = to.parent_path();
        if (!parent.empty()) {
            createDirectories(parent);
        }

        if (std::filesystem::is_symlink(from)) {
            // 处理符号链接
            auto target = std::filesystem::read_symlink(from);
            if (std::filesystem::exists(to)) {
                std::filesystem::remove(to);
            }
            std::filesystem::create_symlink(target, to);
        } else {
            // 复制普通文件
            std::filesystem::copy_file(from, to, 
                std::filesystem::copy_options::overwrite_existing);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "复制文件失败: " << from << " -> " << to 
                  << " - " << e.what() << std::endl;
        return false;
    }
}

std::int64_t FileUtils::getFileSize(const std::filesystem::path& path) {
    try {
        if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
            return std::filesystem::file_size(path);
        }
    } catch (const std::exception& e) {
        std::cerr << "获取文件大小失败: " << path << " - " << e.what() << std::endl;
    }
    return -1;
}

std::filesystem::path FileUtils::getRelativePath(
    const std::filesystem::path& base, 
    const std::filesystem::path& path) {
    try {
        return std::filesystem::relative(path, base);
    } catch (const std::exception& e) {
        std::cerr << "计算相对路径失败: " << base << " / " << path 
                  << " - " << e.what() << std::endl;
        return path;
    }
}

} // namespace backuprestore

