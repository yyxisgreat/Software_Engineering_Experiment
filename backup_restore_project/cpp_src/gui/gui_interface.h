#pragma once

#include <filesystem>
#include <string>
#include <functional>
#include <memory>

namespace backuprestore {

/**
 * @brief GUI 进度回调接口
 * 用于在 GUI 界面中显示备份/还原操作的进度和状态
 */
class ProgressCallback {
public:
    virtual ~ProgressCallback() = default;

    /**
     * @brief 操作开始时的回调
     * @param total_files 总文件数
     * @param operation_name 操作名称（如"备份"、"还原"）
     */
    virtual void onStart(std::size_t total_files, const std::string& operation_name) = 0;

    /**
     * @brief 文件处理进度更新
     * @param current_file 当前处理的文件路径
     * @param current_index 当前文件索引（从1开始）
     * @param total_files 总文件数
     * @param percentage 完成百分比（0-100）
     */
    virtual void onProgress(const std::filesystem::path& current_file,
                           std::size_t current_index,
                           std::size_t total_files,
                           double percentage) = 0;

    /**
     * @brief 文件处理成功
     * @param file_path 文件路径
     */
    virtual void onFileSuccess(const std::filesystem::path& file_path) = 0;

    /**
     * @brief 文件处理失败
     * @param file_path 文件路径
     * @param error_message 错误信息
     */
    virtual void onFileError(const std::filesystem::path& file_path,
                            const std::string& error_message) = 0;

    /**
     * @brief 文件被跳过
     * @param file_path 文件路径
     * @param reason 跳过原因
     */
    virtual void onFileSkipped(const std::filesystem::path& file_path,
                              const std::string& reason) = 0;

    /**
     * @brief 操作完成时的回调
     * @param success_count 成功处理的文件数
     * @param failed_count 失败的文件数
     * @param skipped_count 跳过的文件数
     * @param success 整体是否成功
     */
    virtual void onComplete(std::size_t success_count,
                           std::size_t failed_count,
                           std::size_t skipped_count,
                           bool success) = 0;

    /**
     * @brief 检查是否应该取消操作
     * @return true 如果应该取消
     */
    virtual bool shouldCancel() const = 0;
};

/**
 * @brief GUI 操作接口
 * 提供 GUI 友好的备份和还原操作接口
 */
class GuiOperations {
public:
    /**
     * @brief 执行备份操作（带进度回调）
     * @param source_root 源目录根路径
     * @param repo_path 备份仓库路径
     * @param include_paths 包含路径列表（可选）
     * @param exclude_paths 排除路径列表（可选）
     * @param callback 进度回调接口
     * @return 是否成功
     */
    static bool backupWithProgress(
        const std::filesystem::path& source_root,
        const std::filesystem::path& repo_path,
        const std::vector<std::filesystem::path>& include_paths = {},
        const std::vector<std::filesystem::path>& exclude_paths = {},
        ProgressCallback* callback = nullptr);

    /**
     * @brief 执行还原操作（带进度回调）
     * @param repo_path 备份仓库路径
     * @param target_root 目标目录根路径
     * @param callback 进度回调接口
     * @return 是否成功
     */
    static bool restoreWithProgress(
        const std::filesystem::path& repo_path,
        const std::filesystem::path& target_root,
        ProgressCallback* callback = nullptr);

    /**
     * @brief 列出备份仓库中的文件
     * @param repo_path 备份仓库路径
     * @return 文件列表（相对路径）
     */
    static std::vector<std::filesystem::path> listBackupFiles(
        const std::filesystem::path& repo_path);

    /**
     * @brief 验证备份仓库是否有效
     * @param repo_path 备份仓库路径
     * @return 是否有效
     */
    static bool validateRepository(const std::filesystem::path& repo_path);
};

} // namespace backuprestore

