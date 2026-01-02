#pragma once
#include <filesystem>
#include <string>
#include "algorithms.h"

namespace pkg {

struct Options {
    PackAlg packAlg = PackAlg::HeaderPerFile;
    CompressAlg compressAlg = CompressAlg::None;
    EncryptAlg encryptAlg = EncryptAlg::None;
    std::string password; // encrypt!=None 时必须提供
};

bool export_repo_to_package(const std::filesystem::path& repoDir,
                            const std::filesystem::path& packageFile,
                            const Options& opt);

bool import_package_to_repo(const std::filesystem::path& packageFile,
                            const std::filesystem::path& repoDir,
                            const std::string& password);

} // namespace pkg
