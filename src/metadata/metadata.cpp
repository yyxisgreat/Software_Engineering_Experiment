#include "metadata/metadata.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <array>

#include <sstream>
#include <iostream>

#ifdef _WIN32
// Windows 下很多 POSIX 函数不存在：lstat/utimensat/utimes/AT_*
// 我们用 stat + utime 做简化兼容（足够实验）
#include <io.h>
#endif

namespace backuprestore {

bool Metadata::loadFromFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return false;
    }

    // 检查是否为符号链接（Windows 也支持 is_symlink，但后续 lstat 不一定可用）
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
    struct stat st{};

    // Windows 下：std::filesystem::path::c_str() 是 wchar_t*，不能直接给 stat/chmod
    // 用 path.string() 转成窄字符（UTF-8），MinGW 的 stat 接收 const char*
    const std::string p = path.string();

#ifdef _WIN32
    // Windows 下没有 lstat，直接用 stat（符号链接会当作目标文件处理）
    int result = stat(p.c_str(), &st);
#else
    // POSIX：符号链接用 lstat 获取链接本身信息，否则用 stat
    int result = is_symlink ? lstat(p.c_str(), &st) : stat(p.c_str(), &st);
#endif

    if (result != 0) {
        std::cerr << "获取文件状态失败: " << path << std::endl;
        return false;
    }

    mode = st.st_mode;
    mtime = st.st_mtime;
    uid = st.st_uid;  // 预留（Windows 下意义不大）
    gid = st.st_gid;  // 预留（Windows 下意义不大）

    return true;
}

bool Metadata::applyToFile(const std::filesystem::path& path) const {
    const std::string p = path.string();

    // 应用权限（Windows 下 chmod 只能设置“只读”一类的属性，效果有限，但能跑）
    if (chmod(p.c_str(), mode) != 0) {
        std::cerr << "设置文件权限失败: " << path << std::endl;
        // Windows 下权限可能不支持，我们这里不强制失败（避免影响实验）
#ifndef _WIN32
        return false;
#endif
    }

#ifdef _WIN32
    // Windows 下没有 utimensat/utimes：用 utime 设置修改时间
    // utimbuf 的 actime/modtime 都是 time_t 秒级
    struct utimbuf new_times;
    new_times.actime = static_cast<time_t>(mtime);
    new_times.modtime = static_cast<time_t>(mtime);

    if (utime(p.c_str(), &new_times) != 0) {
        std::cerr << "设置文件时间失败: " << path << std::endl;
        // Windows 下也不强制失败
        // return false;
    }
#else
    // POSIX：优先 utimensat（纳秒），失败则用 utimes（微秒）
    struct timespec times[2];
    times[0].tv_sec = mtime;  // atime
    times[1].tv_sec = mtime;  // mtime
    times[0].tv_nsec = 0;
    times[1].tv_nsec = 0;

    if (utimensat(AT_FDCWD, p.c_str(), times, AT_SYMLINK_NOFOLLOW) != 0) {
        struct timeval tv[2];
        tv[0].tv_sec = mtime;
        tv[0].tv_usec = 0;
        tv[1].tv_sec = mtime;
        tv[1].tv_usec = 0;
        if (utimes(p.c_str(), tv) != 0) {
            std::cerr << "设置文件时间失败: " << path << std::endl;
            return false;
        }
    }
#endif

    // TODO: 应用 uid/gid（需要root权限，Windows 下也没意义）
    // if (chown(p.c_str(), uid, gid) != 0) { ... }

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

        symlink_target = fields[5];
    } catch (const std::exception& e) {
        std::cerr << "反序列化元数据失败: " << e.what() << std::endl;
        return false;
    }

    return true;
}

} // namespace backuprestore
