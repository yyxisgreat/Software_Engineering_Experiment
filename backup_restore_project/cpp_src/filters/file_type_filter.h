#pragma once

#include "filters/filter_base.h"
#include "metadata/filesystem.h"
#include <vector>

namespace backuprestore {

/**
 * @brief 文件类型过滤器
 * 仅当文件类型属于允许列表时包含
 */
class FileTypeFilter : public FilterBase {
public:
    FileTypeFilter() = default;

    /**
     * @brief 添加允许的文件类型
     */
    void addAllowed(FilesystemUtils::FileType type);

    bool shouldInclude(const std::filesystem::path& path) const override;

    Type getType() const override { return Type::FileType; }

private:
    std::vector<FilesystemUtils::FileType> allowed_;
};

} // namespace backuprestore