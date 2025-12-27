#include "core/repository.h"
#include "core/file_utils.h"
#include <sys/stat.h>   // mkfifo
#include <unistd.h>    // symlink
#include <fstream>
#include <iostream>
#include <sstream>

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
        // 先写入索引（保证即使后续不复制数据也能记录元数据）
        index_[relative_path] = metadata;

        auto storage_path = getStoragePath(relative_path);

        // 确保 data/ 下的父目录存在（仅当需要写入数据时才有意义）
        auto parent = storage_path.parent_path();
        if (!parent.empty()) {
            FileUtils::createDirectories(parent);
        }

        // 依据文件类型决定是否需要将“实体数据”写入仓库 data/
        const auto ftype = metadata.file_type;

        // 1) 符号链接：只记录元数据，不在 data/ 中生成任何实体（避免仓库内形成循环 symlink）
        if (ftype == FilesystemUtils::FileType::Symlink || metadata.is_symlink) {
            // 不创建 storage_path，不复制，不跟随链接
            return true;
        }

        // 2) 普通文件：复制实体数据到 data/
        if (ftype == FilesystemUtils::FileType::Regular) {
            return FileUtils::copyFile(source_path, storage_path);
        }

        // 3) FIFO / 设备文件 / socket 等：暂不复制实体数据，仅记录元数据（预留接口）
        // 未来可在此扩展：例如设备文件记录主次设备号，恢复时 mknod
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
        // 1) 从索引获取元数据（先拿元数据决定恢复策略）
        auto it = index_.find(relative_path);
        if (it == index_.end()) {
            std::cerr << "索引中不存在文件: " << relative_path << std::endl;
            return false;
        }
        metadata = it->second;

        const auto ftype = metadata.file_type;
        bool success = true;

        // 2) 确保目标父目录存在
        try {
            auto parent = target_path.parent_path();
            if (!parent.empty()) {
                FileUtils::createDirectories(parent);
            }
        } catch (const std::exception& e) {
            std::cerr << "创建目标父目录失败: " << target_path << " - " << e.what() << std::endl;
            return false;
        }

        // 3) 若目标已存在，先删除（避免覆盖失败/类型冲突）
        try {
            if (std::filesystem::exists(target_path) || std::filesystem::is_symlink(target_path)) {
                std::filesystem::remove(target_path);
            }
        } catch (const std::exception& e) {
            std::cerr << "删除已有目标失败: " << target_path << " - " << e.what() << std::endl;
            return false;
        }

        // 4) 根据文件类型进行恢复
        switch (ftype) {
            case FilesystemUtils::FileType::Regular:
            {
                // 普通文件：必须从仓库 data/ 拷贝实体文件
                auto storage_path = getStoragePath(relative_path);
                if (!std::filesystem::exists(storage_path)) {
                    std::cerr << "仓库中不存在实体数据: " << relative_path
                              << " [ " << storage_path << " ]" << std::endl;
                    return false;
                }
                success = FileUtils::copyFile(storage_path, target_path);
                break;
            }

            case FilesystemUtils::FileType::Symlink:
            {
                // 符号链接：不从 data/ 拷贝，直接按元数据创建链接
                // 若 symlink_target 为空，视为数据损坏
                if (metadata.symlink_target.empty()) {
                    std::cerr << "符号链接目标为空，无法恢复: " << relative_path << std::endl;
                    return false;
                }
                // 创建符号链接：target_path -> symlink_target
                if (::symlink(metadata.symlink_target.c_str(), target_path.c_str()) != 0) {
                    std::cerr << "恢复符号链接失败: " << target_path
                              << " -> " << metadata.symlink_target << std::endl;
                    success = false;
                }
                break;
            }

            case FilesystemUtils::FileType::Fifo:
            {
                // FIFO：按元数据创建管道
                if (::mkfifo(target_path.c_str(), metadata.mode) != 0) {
                    std::cerr << "创建FIFO失败: " << target_path << std::endl;
                    success = false;
                }
                break;
            }

            case FilesystemUtils::FileType::BlockDevice:
            case FilesystemUtils::FileType::CharacterDevice:
            {
                // 设备文件恢复通常需要 root 权限与 mknod，这里预留
                std::cerr << "警告: 设备文件还原未实现，已跳过: " << target_path << std::endl;
                success = true;
                break;
            }

            case FilesystemUtils::FileType::Socket:
            {
                std::cerr << "警告: 套接字文件还原未实现，已跳过: " << target_path << std::endl;
                success = true;
                break;
            }

            case FilesystemUtils::FileType::Directory:
            default:
                // 目录不在这里处理
                success = true;
                break;
        }

        if (!success) {
            return false;
        }

        // 5) 应用元数据（注意：symlink 通常不应 chmod 到“链接目标”，且某些系统对 lutimes/chmod 行为不同）
        // 这里建议：Regular 和 FIFO 应用；Symlink 只要创建成功即可，不强制 applyToFile
        if (ftype == FilesystemUtils::FileType::Regular ||
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


