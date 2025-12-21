#include "storage/compressor.h"
#include "core/file_utils.h"
#include <iostream>

namespace backuprestore {

bool NoCompressor::compress(const std::filesystem::path& input_path,
                            const std::filesystem::path& output_path) {
    // TODO: 实现压缩功能
    // 当前实现：直接复制（无压缩）
    std::cerr << "Compressor::compress() 尚未实现，使用无压缩模式" << std::endl;
    return FileUtils::copyFile(input_path, output_path);
}

bool NoCompressor::decompress(const std::filesystem::path& input_path,
                              const std::filesystem::path& output_path) {
    // TODO: 实现解压功能
    std::cerr << "Compressor::decompress() 尚未实现，使用无压缩模式" << std::endl;
    return FileUtils::copyFile(input_path, output_path);
}

} // namespace backuprestore


