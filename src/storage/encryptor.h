#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace backuprestore {

/**
 * @brief 加密/解密接口（预留）
 * 
 * 加密算法支持：
 * - AES (预留)
 * - 其他对称加密算法 (预留)
 * 
 * TODO: 实现加密功能（不依赖第三方库，可能需要自己实现加密算法）
 */
class Encryptor {
public:
    virtual ~Encryptor() = default;

    /**
     * @brief 加密数据
     * @param input_path 输入文件路径
     * @param output_path 输出加密文件路径
     * @param key 加密密钥
     * @return 是否成功
     */
    virtual bool encrypt(const std::filesystem::path& input_path,
                         const std::filesystem::path& output_path,
                         const std::string& key) = 0;

    /**
     * @brief 解密数据
     * @param input_path 输入加密文件路径
     * @param output_path 输出文件路径
     * @param key 解密密钥
     * @return 是否成功
     */
    virtual bool decrypt(const std::filesystem::path& input_path,
                         const std::filesystem::path& output_path,
                         const std::string& key) = 0;

    /**
     * @brief 设置加密算法
     */
    virtual void setAlgorithm(const std::string& algorithm) = 0;
};

/**
 * @brief 无加密实现（占位符）
 */
class NoEncryptor : public Encryptor {
public:
    bool encrypt(const std::filesystem::path& input_path,
                 const std::filesystem::path& output_path,
                 const std::string& key) override;

    bool decrypt(const std::filesystem::path& input_path,
                 const std::filesystem::path& output_path,
                 const std::string& key) override;

    void setAlgorithm(const std::string& algorithm) override {}
};

} // namespace backuprestore


