#include "core/repository.h"
#include "core/file_utils.h"
// C++ standard headers
#include <fstream>
#include <iostream>
#include <sstream>
// POSIX headers for special file operations
#include <sys/stat.h>   // mkfifo, mode constants
#include <unistd.h>     // POSIX API such as unlink
#include "metadata/filesystem.h"

namespace backuprestore {

Repository::Repository(const std::filesystem::path& repo_path)
    : repo_path_(repo_path),
      data_dir_(repo_path / "data"),
      index_file_(repo_path / "index.txt") {
}

bool Repository::initialize() {
    try {
        FileUtils::createDirectories(repo_path_);
        FileUtils::createDirectories(data_dir_);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "初始化仓库失败: " << e.what() << std::endl;
        return false;
    }
}

std::filesystem::path Repository::getStoragePath(const std::filesystem::path& relative_path) const {
    // 将相对路径转换为仓库中的存储路径
    // 使用相对路径本身作为存储路径（可以添加哈希等优化）
    return data_dir_ / relative_path;
}

bool Repository::storeFile(const std::filesystem::path& source_path,
                           const std::filesystem::path& relative_path,
                           const Metadata& metadata) {
    try {
        auto storage_path = getStoragePath(relative_path);

        // 检查并创建父目录
        auto parent = storage_path.parent_path();
        if (!parent.empty()) {
            FileUtils::createDirectories(parent);
        }

        // 根据文件类型决定如何存储
        auto ftype = metadata.file_type;
        bool copy_ok = true;
        switch (ftype) {
            case FilesystemUtils::FileType::Regular:
            case FilesystemUtils::FileType::Symlink:
                // 对于普通文件和符号链接直接复制
                copy_ok = FileUtils::copyFile(source_path, storage_path);
                break;
            default:
                // 其他类型暂不复制实际数据，仅记录元数据
                // 在还原时根据元数据自行创建
                copy_ok = true;
                break;
        }
        if (!copy_ok) {
            return false;
        }

        // 保存元数据到索引
        index_[relative_path] = metadata;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "存储文件失败: " << source_path << " - " << e.what() << std::endl;
        return false;
    }
}

bool Repository::restoreFile(const std::filesystem::path& relative_path,
                             const std::filesystem::path& target_path,
                             Metadata& metadata) {
    try {
        auto storage_path = getStoragePath(relative_path);

        // Determine file type from metadata (we will load it below).
        FilesystemUtils::FileType ftype_for_existence_check = FilesystemUtils::FileType::Regular;
        {
            // Try to peek metadata in index_ if available
            auto it_meta = index_.find(relative_path);
            if (it_meta != index_.end()) {
                ftype_for_existence_check = it_meta->second.file_type;
            }
        }

        // For regular files and symlinks we need the storage file to exist; for other types (FIFO, devices,
        // sockets, directories) we do not expect a data file and should not fail if it is missing.
        if ((ftype_for_existence_check == FilesystemUtils::FileType::Regular ||
             ftype_for_existence_check == FilesystemUtils::FileType::Symlink) &&
            !std::filesystem::exists(storage_path)) {
            std::cerr << "仓库中不存在文件: " << relative_path << std::endl;
            return false;
        }

        // 从索引获取元数据
        auto it = index_.find(relative_path);
        if (it == index_.end()) {
            std::cerr << "索引中不存在文件: " << relative_path << std::endl;
            return false;
        }
        metadata = it->second;

        // 根据文件类型创建目标文件
        bool success = true;
        auto ftype = metadata.file_type;
        switch (ftype) {
            case FilesystemUtils::FileType::Regular:
            case FilesystemUtils::FileType::Symlink:
            {
                // 复制文件或符号链接
                success = FileUtils::copyFile(storage_path, target_path);
                break;
            }
            case FilesystemUtils::FileType::Fifo:
            {
                // 创建FIFO管道
                try {
                    auto parent = target_path.parent_path();
                    if (!parent.empty()) {
                        FileUtils::createDirectories(parent);
                    }
                    if (std::filesystem::exists(target_path)) {
                        std::filesystem::remove(target_path);
                    }
                    if (mkfifo(target_path.c_str(), metadata.mode) != 0) {
                        std::cerr << "创建FIFO失败: " << target_path << std::endl;
                        success = false;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "创建FIFO异常: " << target_path << " - " << e.what() << std::endl;
                    success = false;
                }
                break;
            }
            case FilesystemUtils::FileType::BlockDevice:
            case FilesystemUtils::FileType::CharacterDevice:
            {
                // 创建设备文件需要root权限，预留实现
                std::cerr << "警告: 设备文件还原未实现，已跳过: " << target_path << std::endl;
                success = true;
                break;
            }
            case FilesystemUtils::FileType::Socket:
            {
                // 套接字无法离线还原，预留
                std::cerr << "警告: 套接字文件还原未实现，已跳过: " << target_path << std::endl;
                success = true;
                break;
            }
            case FilesystemUtils::FileType::Directory:
            {
                // 不处理，目录结构由其他逻辑创建
                success = true;
                break;
            }
            default:
                success = true;
                break;
        }
        if (!success) {
            return false;
        }
        // 对于复制/创建的文件，应用元数据
        // 设备文件/套接字跳过 applyToFile，因为创建未完成
        if (ftype == FilesystemUtils::FileType::Regular ||
            ftype == FilesystemUtils::FileType::Symlink ||
            ftype == FilesystemUtils::FileType::Fifo) {
            if (!metadata.applyToFile(target_path)) {
                std::cerr << "警告: 应用元数据失败: " << target_path << std::endl;
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "恢复文件失败: " << relative_path << " - " << e.what() << std::endl;
        return false;
    }
}

bool Repository::saveIndex() {
    try {
        std::ofstream ofs(index_file_);
        if (!ofs.is_open()) {
            std::cerr << "无法打开索引文件: " << index_file_ << std::endl;
            return false;
        }

        for (const auto& [path, metadata] : index_) {
            ofs << path.string() << "\t" << metadata.serialize() << "\n";
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存索引失败: " << e.what() << std::endl;
        return false;
    }
}

bool Repository::loadIndex() {
    try {

        // Ensure the repository and data directories exist. In restore-only scenarios,
        // the repository may not have been initialized via initialize(), which
        // results in missing directories (e.g. data/) causing restore to fail.
        try {
            FileUtils::createDirectories(repo_path_);
            FileUtils::createDirectories(data_dir_);
        } catch (...) {
            // silently ignore directory creation failures; they'll be reported later
        }

        if (!std::filesystem::exists(index_file_)) {
            return true;  // 索引文件不存在，返回成功（空索引）
        }

        std::ifstream ifs(index_file_);
        if (!ifs.is_open()) {
            std::cerr << "无法打开索引文件: " << index_file_ << std::endl;
            return false;
        }

        index_.clear();
        std::string line;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;

            size_t tab_pos = line.find('\t');
            if (tab_pos == std::string::npos) continue;

            std::string path_str = line.substr(0, tab_pos);
            std::string metadata_str = line.substr(tab_pos + 1);

            Metadata metadata;
            if (metadata.deserialize(metadata_str)) {
                index_[path_str] = metadata;
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "加载索引失败: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::filesystem::path> Repository::listFiles() const {
    std::vector<std::filesystem::path> files;
    for (const auto& [path, _] : index_) {
        files.push_back(path);
    }
    return files;
}

bool Repository::getMetadata(const std::filesystem::path& relative_path, Metadata& metadata) const {
    auto it = index_.find(relative_path);
    if (it == index_.end()) {
        return false;
    }
    metadata = it->second;
    return true;
}

} // namespace backuprestore


