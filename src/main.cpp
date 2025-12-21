#include <iostream>
#include <string>
#include <memory>
#include <filesystem>
#include "core/repository.h"
#include "core/backup.h"
#include "core/restore.h"
#include "filters/path_filter.h"

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
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " backup /home/user/docs /backup/repo" << std::endl;
    std::cout << "  " << program_name << " backup /home/user --include /home/user/docs --exclude /home/user/tmp" << std::endl;
    std::cout << "  " << program_name << " restore /backup/repo /home/user/restored" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printUsage(argv[0]);
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

        // 解析过滤器选项
        std::unique_ptr<PathFilter> filter = std::make_unique<PathFilter>();
        bool has_filter = false;
        for (int i = 4; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--include" && i + 1 < argc) {
                filter->addInclude(argv[++i]);
                has_filter = true;
            } else if (arg == "--exclude" && i + 1 < argc) {
                filter->addExclude(argv[++i]);
                has_filter = true;
            }
        }

        // 创建仓库
        auto repo = std::make_shared<Repository>(repo_path);
        if (!repo->initialize()) {
            std::cerr << "初始化仓库失败" << std::endl;
            return 1;
        }

        // 执行备份
        Backup backup(repo);
        const FilterBase* filter_ptr = has_filter ? filter.get() : nullptr;
        
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

