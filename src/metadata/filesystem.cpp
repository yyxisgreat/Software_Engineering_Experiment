#include "metadata/filesystem.h"

#include <sys/stat.h>
#include <stdexcept>

// ---------------- Cross-platform S_IS* fallback ----------------
#ifndef S_ISREG
#  define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
#  define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
#  define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISFIFO
#  define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
// ⚠️ Windows/MinGW 可能没有 S_IFSOCK，所以这里不定义 S_ISSOCK
// ----------------------------------------------------------------

namespace backuprestore {

FilesystemUtils::FileType FilesystemUtils::getFileType(const std::filesystem::path& path) {
    struct stat st{};
    const std::string p = path.string();

#ifdef _WIN32
    // Windows 没有 lstat：用 stat（符号链接会当作目标）
    int ret = stat(p.c_str(), &st);
#else
    int ret = lstat(p.c_str(), &st);
#endif

    if (ret != 0) {
        throw std::runtime_error("stat/lstat failed: " + p);
    }

    // 符号链接判断（仅 POSIX）
#ifndef _WIN32
#ifdef S_IFLNK
    if ((st.st_mode & S_IFMT) == S_IFLNK) {
        return FileType::Symlink;
    }
#endif
#endif

    if (S_ISREG(st.st_mode)) return FileType::Regular;
    if (S_ISDIR(st.st_mode)) return FileType::Directory;
    if (S_ISBLK(st.st_mode)) return FileType::BlockDevice;
    if (S_ISCHR(st.st_mode)) return FileType::CharacterDevice;
    if (S_ISFIFO(st.st_mode)) return FileType::Fifo;

    // Socket：MinGW/Windows 通常不支持这种文件类型宏，因此跳过
#ifdef S_IFSOCK
    if (((st.st_mode) & S_IFMT) == S_IFSOCK) return FileType::Socket;
#endif

    // 其他类型：认为不支持
    throw std::runtime_error("Unsupported file type: " + p);
}

bool FilesystemUtils::isBackupSupported(FileType type) {
    switch (type) {
        case FileType::Regular:
        case FileType::Directory:
        case FileType::Symlink:
            return true;
        default:
            return false;
    }
}

} // namespace backuprestore
