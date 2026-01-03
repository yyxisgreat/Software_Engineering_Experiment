#include "metadata/filesystem.h"
#include <sys/stat.h>

namespace backuprestore {

FilesystemUtils::FileType FilesystemUtils::getFileType(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return FileType::Regular;
    }

    if (std::filesystem::is_symlink(path)) {
        return FileType::Symlink;
    }

    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            return FileType::Regular;
        } else if (S_ISDIR(st.st_mode)) {
            return FileType::Directory;
        } else if (S_ISBLK(st.st_mode)) {
            return FileType::BlockDevice;
        } else if (S_ISCHR(st.st_mode)) {
            return FileType::CharacterDevice;
        } else if (S_ISFIFO(st.st_mode)) {
            return FileType::Fifo;
        } else if (S_ISSOCK(st.st_mode)) {
            return FileType::Socket;
        }
    }

    return FileType::Regular;
}

bool FilesystemUtils::isBackupSupported(FilesystemUtils::FileType type) {
    switch (type) {
        case FileType::Regular:
        case FileType::Symlink:
            return true;
        case FileType::Directory:
            return true;  // 目录结构本身，不需要单独备份
        // TODO: 其他文件类型需要特殊处理
        default:
            return false;
    }
}

} // namespace backuprestore


