#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace backuprestore {

/**
 * @brief 文件工具类，提供文件操作的封装
 */
class FileUtils {
public:
    /**
     * @brief 递归获取目录下的所有文件路径
     * @param root 根目录路径
     * @param files 输出文件路径列表
     */
    static void getFilesRecursive(const std::filesystem::path& root, 
                                   std::vector<std::filesystem::path>& files);

    /**
     * @brief 创建目录（递归创建所有父目录）
     * @param path 目录路径
     * @return 是否成功
     */
    static bool createDirectories(const std::filesystem::path& path);

    /**
     * @brief 复制文件
     * @param from 源文件路径
     * @param to 目标文件路径
     * @return 是否成功
     */
    static bool copyFile(const std::filesystem::path& from, 
                         const std::filesystem::path& to);

    /**
     * @brief 获取文件大小
     * @param path 文件路径
     * @return 文件大小（字节），失败返回-1
     */
    static std::int64_t getFileSize(const std::filesystem::path& path);

    /**
     * @brief 计算相对路径
     * @param base 基础路径
     * @param path 完整路径
     * @return 相对路径
     */
    static std::filesystem::path getRelativePath(
        const std::filesystem::path& base, 
        const std::filesystem::path& path);
};

} // namespace backuprestore


