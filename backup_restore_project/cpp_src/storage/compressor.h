#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace backuprestore {

/**
 * @brief 压缩/解压接口（预留）
 * 
 * 压缩算法支持：
 * - gzip (预留)
 * - bzip2 (预留)
 * - xz (预留)
 * 
 * TODO: 实现压缩功能（不依赖第三方库，可能需要自己实现或使用系统命令）
 */
class Compressor {
public:
    virtual ~Compressor() = default;

    /**
     * @brief 压缩数据
     * @param input_path 输入文件路径
     * @param output_path 输出压缩文件路径
     * @return 是否成功
     */
    virtual bool compress(const std::filesystem::path& input_path,
                          const std::filesystem::path& output_path) = 0;

    /**
     * @brief 解压数据
     * @param input_path 输入压缩文件路径
     * @param output_path 输出文件路径
     * @return 是否成功
     */
    virtual bool decompress(const std::filesystem::path& input_path,
                            const std::filesystem::path& output_path) = 0;

    /**
     * @brief 获取压缩级别（1-9）
     */
    virtual int getCompressionLevel() const = 0;

    /**
     * @brief 设置压缩级别
     */
    virtual void setCompressionLevel(int level) = 0;
};

/**
 * @brief 无压缩实现（占位符）
 */
class NoCompressor : public Compressor {
public:
    bool compress(const std::filesystem::path& input_path,
                  const std::filesystem::path& output_path) override;

    bool decompress(const std::filesystem::path& input_path,
                    const std::filesystem::path& output_path) override;

    int getCompressionLevel() const override { return 0; }
    void setCompressionLevel(int level) override {}
};

} // namespace backuprestore


