#include "storage/encryptor.h"
#include "core/file_utils.h"
#include <iostream>

namespace backuprestore {

bool NoEncryptor::encrypt(const std::filesystem::path& input_path,
                          const std::filesystem::path& output_path,
                          const std::string& key) {
    // TODO: 实现加密功能
    std::cerr << "Encryptor::encrypt() 尚未实现，使用无加密模式" << std::endl;
    (void)key;  // 未使用参数
    return FileUtils::copyFile(input_path, output_path);
}

bool NoEncryptor::decrypt(const std::filesystem::path& input_path,
                          const std::filesystem::path& output_path,
                          const std::string& key) {
    // TODO: 实现解密功能
    std::cerr << "Encryptor::decrypt() 尚未实现，使用无加密模式" << std::endl;
    (void)key;  // 未使用参数
    return FileUtils::copyFile(input_path, output_path);
}

} // namespace backuprestore


