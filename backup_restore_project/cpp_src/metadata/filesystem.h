#pragma once

#include <string>
#include <filesystem>

namespace backuprestore {

/**
 * @brief 文件系统相关工具
 * 预留：支持更多文件类型（管道/设备等）
 */
class FilesystemUtils {
public:
    enum class FileType {
        Regular,
        Directory,
        Symlink,
        BlockDevice,    // 预留
        CharacterDevice, // 预留
        Fifo,           // 预留
        Socket          // 预留
    };

    /**
     * @brief 获取文件类型
     */
    static FileType getFileType(const std::filesystem::path& path);

    /**
     * @brief 检查文件类型是否支持备份
     */
    static bool isBackupSupported(FileType type);
};

} // namespace backuprestore


