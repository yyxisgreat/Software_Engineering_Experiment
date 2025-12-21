#include "metadata/metadata.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>

namespace backuprestore {

bool Metadata::loadFromFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return false;
    }

    // 检查是否为符号链接
    is_symlink = std::filesystem::is_symlink(path);
    if (is_symlink) {
        try {
            symlink_target = std::filesystem::read_symlink(path).string();
        } catch (const std::exception& e) {
            std::cerr << "读取符号链接目标失败: " << path << " - " << e.what() << std::endl;
            return false;
        }
    }

    return loadFromStat(path);
}

bool Metadata::loadFromStat(const std::filesystem::path& path) {
    struct stat st;
    // 对于符号链接，使用lstat获取链接本身的信息
    int result = is_symlink ? lstat(path.c_str(), &st) : stat(path.c_str(), &st);
    if (result != 0) {
        std::cerr << "获取文件状态失败: " << path << std::endl;
        return false;
    }

    mode = st.st_mode;
    mtime = st.st_mtime;
    uid = st.st_uid;  // 预留
    gid = st.st_gid;  // 预留

    return true;
}

bool Metadata::applyToFile(const std::filesystem::path& path) const {
    // 应用权限
    if (chmod(path.c_str(), mode) != 0) {
        std::cerr << "设置文件权限失败: " << path << std::endl;
        return false;
    }

    // 应用修改时间
    struct timespec times[2];
    times[0].tv_sec = mtime;  // atime
    times[1].tv_sec = mtime;  // mtime
    times[0].tv_nsec = 0;
    times[1].tv_nsec = 0;
    
    if (utimensat(AT_FDCWD, path.c_str(), times, AT_SYMLINK_NOFOLLOW) != 0) {
        // 如果utimensat失败，尝试使用utimes（兼容性备用）
        struct timeval tv[2];
        tv[0].tv_sec = mtime;
        tv[0].tv_usec = 0;
        tv[1].tv_sec = mtime;
        tv[1].tv_usec = 0;
        if (utimes(path.c_str(), tv) != 0) {
            std::cerr << "设置文件时间失败: " << path << std::endl;
            return false;
        }
    }

    // TODO: 应用 uid/gid（需要root权限）
    // if (chown(path.c_str(), uid, gid) != 0) { ... }

    return true;
}

std::string Metadata::serialize() const {
    std::ostringstream oss;
    oss << mode << ":" << mtime << ":" << uid << ":" << gid 
        << ":" << (is_symlink ? 1 : 0) << ":" << symlink_target;
    return oss.str();
}

bool Metadata::deserialize(const std::string& data) {
    // 期望格式：
    // mode:mtime:uid:gid:is_symlink:symlink_target(可包含冒号)
    std::array<std::string, 6> fields;
    size_t start = 0;

    for (int i = 0; i < 5; ++i) {
        size_t pos = data.find(':', start);
        if (pos == std::string::npos) return false;
        fields[i] = data.substr(start, pos - start);
        start = pos + 1;
    }
    fields[5] = data.substr(start); // 剩余全部

    try {
        mode = std::stoul(fields[0]);
        mtime = std::stoll(fields[1]);
        uid  = std::stoul(fields[2]);
        gid  = std::stoul(fields[3]);

        int s = std::stoi(fields[4]);
        if (s != 0 && s != 1) return false;
        is_symlink = (s == 1);

        symlink_target = fields[5]; // 可为空（看你是否允许）
    } catch (const std::exception& e) {
        std::cerr << "反序列化元数据失败: " << e.what() << std::endl;
        return false;
    }

    return true;
}

} // namespace backuprestore

