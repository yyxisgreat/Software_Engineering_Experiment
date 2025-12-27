#include <iostream>
#include <string>
#include <memory>
#include <filesystem>
#include "core/repository.h"
#include "core/backup.h"
#include "core/restore.h"
#include "filters/path_filter.h"
#include "filters/filter_chain.h"
#include "filters/file_type_filter.h"
#include "filters/name_filter.h"
#include "filters/time_filter.h"
#include "filters/size_filter.h"
#include "filters/user_filter.h"

using namespace backuprestore;

void printUsage(const char* program_name) {
    std::cout << "用法: " << program_name << " <命令> [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "命令:" << std::endl;
    std::cout << "  backup <源目录> <仓库路径>         备份目录到仓库" << std::endl;
    std::cout << "  restore <仓库路径> <目标目录>      从仓库还原到目标目录" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --include <路径>    包含路径（可多次指定）" << std::endl;
    std::cout << "  --exclude <路径>    排除路径（可多次指定）" << std::endl;
    std::cout << "  --type <类型>       按文件类型过滤，类型包括 regular, symlink, fifo, block, char, socket" << std::endl;
    std::cout << "  --name-contains <关键字> 按文件名包含关键字过滤（可多次指定）" << std::endl;
    std::cout << "  --mtime-after <时间戳>    按修改时间下界过滤（Unix时间戳）" << std::endl;
    std::cout << "  --mtime-before <时间戳>   按修改时间上界过滤（Unix时间戳）" << std::endl;
    std::cout << "  --min-size <字节>        最小文件大小过滤" << std::endl;
    std::cout << "  --max-size <字节>        最大文件大小过滤" << std::endl;
    std::cout << "  --uid <用户ID>           按文件所属用户ID过滤" << std::endl;
    std::cout << "  --gid <组ID>             按文件所属组ID过滤" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " backup /home/user/docs /backup/repo" << std::endl;
    std::cout << "  " << program_name << " backup /home/user --include /home/user/docs --exclude /home/user/tmp" << std::endl;
    std::cout << "  " << program_name << " restore /backup/repo /home/user/restored" << std::endl;
}

int main(int argc, char* argv[]) {
    // at least 4 arguments: program name, command, source_root, repo_path
    if (argc < 4) {
        printUsage(argv[0]); // print usage and exit
        return 1;
    }

    std::string command = argv[1];

    if (command == "backup") {
        if (argc < 4) {
            std::cerr << "错误: backup命令需要源目录和仓库路径" << std::endl;
            return 1;
        }

        std::filesystem::path source_root = argv[2];
        std::filesystem::path repo_path = argv[3];

        // 创建过滤器链
        std::unique_ptr<FilterChain> chain = std::make_unique<FilterChain>();

        // 单独的过滤器实例
        auto pathFilter = std::make_unique<PathFilter>();
        bool pathUsed = false;
        auto typeFilter = std::make_unique<FileTypeFilter>();
        bool typeUsed = false;
        auto nameFilter = std::make_unique<NameFilter>();
        bool nameUsed = false;
        auto timeFilter = std::make_unique<TimeFilter>();
        bool timeUsed = false;
        auto sizeFilter = std::make_unique<SizeFilter>();
        bool sizeUsed = false;
        auto userFilter = std::make_unique<UserFilter>();
        bool userUsed = false;

        // 解析过滤器选项
        for (int i = 4; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--include" && i + 1 < argc) {
                pathFilter->addInclude(argv[++i]);
                pathUsed = true;
            } else if (arg == "--exclude" && i + 1 < argc) {
                pathFilter->addExclude(argv[++i]);
                pathUsed = true;
            } else if (arg == "--type" && i + 1 < argc) {
                std::string t = argv[++i];
                if (t == "regular") {
                    typeFilter->addAllowed(FilesystemUtils::FileType::Regular);
                } else if (t == "symlink") {
                    typeFilter->addAllowed(FilesystemUtils::FileType::Symlink);
                } else if (t == "fifo") {
                    typeFilter->addAllowed(FilesystemUtils::FileType::Fifo);
                } else if (t == "block") {
                    typeFilter->addAllowed(FilesystemUtils::FileType::BlockDevice);
                } else if (t == "char") {
                    typeFilter->addAllowed(FilesystemUtils::FileType::CharacterDevice);
                } else if (t == "socket") {
                    typeFilter->addAllowed(FilesystemUtils::FileType::Socket);
                } else {
                    std::cerr << "警告: 未知的文件类型过滤: " << t << std::endl;
                }
                typeUsed = true;
            } else if (arg == "--name-contains" && i + 1 < argc) {
                nameFilter->addContains(argv[++i]);
                nameUsed = true;
            } else if (arg == "--mtime-after" && i + 1 < argc) {
                std::time_t t = std::stoll(argv[++i]);
                timeFilter->setAfter(t);
                timeUsed = true;
            } else if (arg == "--mtime-before" && i + 1 < argc) {
                std::time_t t = std::stoll(argv[++i]);
                timeFilter->setBefore(t);
                timeUsed = true;
            } else if (arg == "--min-size" && i + 1 < argc) {
                std::int64_t s = std::stoll(argv[++i]);
                sizeFilter->setMinSize(s);
                sizeUsed = true;
            } else if (arg == "--max-size" && i + 1 < argc) {
                std::int64_t s = std::stoll(argv[++i]);
                sizeFilter->setMaxSize(s);
                sizeUsed = true;
            } else if (arg == "--uid" && i + 1 < argc) {
                std::uint32_t id = static_cast<std::uint32_t>(std::stoul(argv[++i]));
                userFilter->setUid(id);
                userUsed = true;
            } else if (arg == "--gid" && i + 1 < argc) {
                std::uint32_t id = static_cast<std::uint32_t>(std::stoul(argv[++i]));
                userFilter->setGid(id);
                userUsed = true;
            }
        }

        // 将使用过的过滤器加入链
        if (pathUsed) {
            chain->addFilter(std::move(pathFilter));
        }
        if (typeUsed) {
            chain->addFilter(std::move(typeFilter));
        }
        if (nameUsed) {
            chain->addFilter(std::move(nameFilter));
        }
        if (timeUsed) {
            chain->addFilter(std::move(timeFilter));
        }
        if (sizeUsed) {
            chain->addFilter(std::move(sizeFilter));
        }
        if (userUsed) {
            chain->addFilter(std::move(userFilter));
        }

        const FilterBase* filter_ptr = nullptr;
        // 判断是否有过滤器添加
        if ((pathUsed || typeUsed || nameUsed || timeUsed || sizeUsed || userUsed)) {
            filter_ptr = chain.get();
        }

        // 创建仓库
        auto repo = std::make_shared<Repository>(repo_path);
        if (!repo->initialize()) {
            std::cerr << "初始化仓库失败" << std::endl;
            return 1;
        }

        // 执行备份
        Backup backup(repo);

        if (!backup.execute(source_root, filter_ptr)) {
            std::cerr << "备份失败" << std::endl;
            return 1;
        }

        std::cout << "备份成功完成" << std::endl;

    } else if (command == "restore") {
        if (argc < 4) {
            std::cerr << "错误: restore命令需要仓库路径和目标目录" << std::endl;
            return 1;
        }

        std::filesystem::path repo_path = argv[2];
        std::filesystem::path target_root = argv[3];

        // 创建仓库
        auto repo = std::make_shared<Repository>(repo_path);
        if (!repo->loadIndex()) {
            std::cerr << "加载仓库索引失败" << std::endl;
            return 1;
        }

        // 执行还原
        Restore restore(repo);
        if (!restore.execute(target_root)) {
            std::cerr << "还原失败" << std::endl;
            return 1;
        }

        std::cout << "还原成功完成" << std::endl;

    } else {
        std::cerr << "错误: 未知命令 '" << command << "'" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}

