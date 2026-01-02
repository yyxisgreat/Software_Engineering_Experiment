#include <iostream>
#include <string>
#include <memory>
#include <filesystem>

#include "core/repository.h"
#include "core/backup.h"
#include "core/restore.h"
#include "filters/path_filter.h"
#ifdef _WIN32
#include <windows.h>
#endif


// 新增：export/import 功能
#include "storage/package/package_export.h"

using namespace backuprestore;

static void printUsage(const char* program_name) {
    std::cout << "用法: " << program_name << " <命令> [选项]" << std::endl;
    std::cout << std::endl;

    std::cout << "命令:" << std::endl;
    std::cout << "  backup  <源目录> <仓库路径>                         备份目录到仓库" << std::endl;
    std::cout << "  restore <仓库路径> <目标目录>                      从仓库还原到目标目录" << std::endl;
    std::cout << "  export  <仓库路径> <输出包文件.sepkg>              将仓库目录打包成单文件" << std::endl;
    std::cout << "  import  <包文件.sepkg> <仓库路径>                  从单文件包恢复仓库目录" << std::endl;
    std::cout << std::endl;

    std::cout << "backup 选项:" << std::endl;
    std::cout << "  --include <路径>    包含路径（可多次指定）" << std::endl;
    std::cout << "  --exclude <路径>    排除路径（可多次指定）" << std::endl;
    std::cout << std::endl;

    std::cout << "export 选项:" << std::endl;
    std::cout << "  --pack header|toc          打包算法（默认 header）" << std::endl;
    std::cout << "  --compress none|rle        压缩算法（默认 none）" << std::endl;
    std::cout << "  --encrypt none|xor|rc4     加密算法（默认 none）" << std::endl;
    std::cout << "  --password <密码>          加密/解密密码（encrypt!=none 必须）" << std::endl;
    std::cout << std::endl;

    std::cout << "import 选项:" << std::endl;
    std::cout << "  --password <密码>          解密密码（包被加密时必须）" << std::endl;
    std::cout << std::endl;

    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " backup  .\\test\\source .\\test\\repo" << std::endl;
    std::cout << "  " << program_name << " restore .\\test\\repo   .\\test\\target" << std::endl;
    std::cout << "  " << program_name << " export  .\\test\\repo   .\\test\\repo_full.sepkg --pack toc --compress rle --encrypt rc4 --password 123456" << std::endl;
    std::cout << "  " << program_name << " import  .\\test\\repo_full.sepkg .\\test\\repo --password 123456" << std::endl;
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    // ===========================
    // backup
    // ===========================
    if (command == "backup") {
        if (argc < 4) {
            std::cerr << "错误: backup命令需要源目录和仓库路径" << std::endl;
            printUsage(argv[0]);
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
        return 0;
    }

    // ===========================
    // restore
    // ===========================
    if (command == "restore") {
        if (argc < 4) {
            std::cerr << "错误: restore命令需要仓库路径和目标目录" << std::endl;
            printUsage(argv[0]);
            return 1;
        }

        std::filesystem::path repo_path = argv[2];
        std::filesystem::path target_root = argv[3];

        auto repo = std::make_shared<Repository>(repo_path);
        if (!repo->loadIndex()) {
            std::cerr << "加载仓库索引失败" << std::endl;
            return 1;
        }

        Restore restore(repo);
        if (!restore.execute(target_root)) {
            std::cerr << "还原失败" << std::endl;
            return 1;
        }

        std::cout << "还原成功完成" << std::endl;
        return 0;
    }

    // ===========================
    // export
    // ===========================
    if (command == "export") {
        if (argc < 4) {
            std::cerr << "错误: export命令需要仓库路径和输出包文件" << std::endl;
            printUsage(argv[0]);
            return 1;
        }

        std::filesystem::path repoDir = argv[2];
        std::filesystem::path pkgFile = argv[3];

        pkg::Options opt;

        // 解析 export 参数
        for (int i = 4; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--pack" && i + 1 < argc) {
                opt.packAlg = pkg::parsePack(argv[++i]);
            } else if (a == "--compress" && i + 1 < argc) {
                opt.compressAlg = pkg::parseCompress(argv[++i]);
            } else if (a == "--encrypt" && i + 1 < argc) {
                opt.encryptAlg = pkg::parseEncrypt(argv[++i]);
            } else if (a == "--password" && i + 1 < argc) {
                opt.password = argv[++i];
            } else {
                std::cerr << "警告: 未识别参数: " << a << std::endl;
            }
        }

        try {
            pkg::export_repo_to_package(repoDir, pkgFile, opt);
            std::cout << "Export OK: " << pkgFile << std::endl;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Export FAIL: " << e.what() << std::endl;
            return 1;
        }
    }

    // ===========================
    // import
    // ===========================
    if (command == "import") {
        if (argc < 4) {
            std::cerr << "错误: import命令需要包文件和仓库路径" << std::endl;
            printUsage(argv[0]);
            return 1;
        }

        std::filesystem::path pkgFile = argv[2];
        std::filesystem::path repoDir = argv[3];
        std::string password;

        // 解析 import 参数
        for (int i = 4; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--password" && i + 1 < argc) {
                password = argv[++i];
            } else {
                std::cerr << "警告: 未识别参数: " << a << std::endl;
            }
        }

        try {
            pkg::import_package_to_repo(pkgFile, repoDir, password);
            std::cout << "Import OK: " << repoDir << std::endl;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Import FAIL: " << e.what() << std::endl;
            return 1;
        }
    }

    // ===========================
    // unknown
    // ===========================
    std::cerr << "错误: 未知命令 '" << command << "'" << std::endl;
    printUsage(argv[0]);
    return 1;
}
