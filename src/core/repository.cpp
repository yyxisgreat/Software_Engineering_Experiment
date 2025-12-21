#include "core/repository.h"
#include "core/file_utils.h"
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
        auto storage_path = getStoragePath(relative_path);
        
        // 复制文件
        if (!FileUtils::copyFile(source_path, storage_path)) {
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
        
        if (!std::filesystem::exists(storage_path)) {
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

        // 恢复文件
        if (!FileUtils::copyFile(storage_path, target_path)) {
            return false;
        }

        // 应用元数据
        if (!metadata.applyToFile(target_path)) {
            std::cerr << "警告: 应用元数据失败: " << target_path << std::endl;
            // 不返回false，文件已复制成功
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


