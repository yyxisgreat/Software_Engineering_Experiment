# 目录树备份/还原软件

一个基于 C++17 的 Linux 目录树备份与还原工具，支持文件数据备份、元数据保存（权限和时间戳）以及路径过滤功能。

## 功能特性

### 基础功能（已实现）

- ✅ **数据备份**: 将目录树中的文件数据保存到指定备份仓库
- ✅ **数据还原**: 将备份仓库中的文件数据恢复到指定位置
- ✅ **符号链接支持**: 最小实现符号链接的备份和还原
- ✅ **元数据支持**: 保存和恢复文件权限（mode）和修改时间（mtime）
- ✅ **路径过滤**: 支持 include/exclude 路径规则

### 扩展功能（接口预留）

- 🔄 **文件类型支持**: 预留管道/设备文件等接口（符号链接已实现）
- 🔄 **元数据扩展**: uid/gid 预留接口（当前实现 mode + mtime）
- 🔄 **自定义备份**: 路径过滤已实现，其他条件（类型/名字/时间/尺寸/用户）预留
- 📋 **打包/解包**: PackStore 接口预留（数据格式说明见代码注释）
- 📋 **压缩/解压**: Compressor 接口预留
- 📋 **加密/解密**: Encryptor 接口预留

## 编译要求

- CMake 3.10 或更高版本
- C++17 编译器（GCC 7+ 或 Clang 5+）
- Linux 系统（需要 POSIX 系统调用支持）

## 编译方法

```bash
# 创建构建目录
mkdir build
cd build

# 配置并编译
cmake ..
make

# 可执行文件位于 build/backup-restore
```

## 使用方法

### 备份目录

```bash
# 基本备份
./backup-restore backup ../test/source ../test/repo

# 带过滤规则的备份
./backup-restore backup /home/user --include /home/user/docs --exclude /home/user/tmp /backup/repo
```

### 还原目录

```bash
./backup-restore restore ../test/repo ../test/target
```

### 示例

```bash
# 备份文档目录
./backup-restore backup /home/user/Documents /backup/mybackup

# 还原到新位置
./backup-restore restore /backup/mybackup /home/user/Restored

# 备份时排除临时文件
./backup-restore backup /home/user \
    --exclude /home/user/.cache \
    --exclude /home/user/tmp \
    /backup/userbackup
```

## 项目结构

```
.
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 本文档
└── src/
    ├── main.cpp            # CLI 入口程序
    ├── core/               # 核心功能模块
    │   ├── backup.cpp/h    # 备份操作
    │   ├── restore.cpp/h   # 还原操作
    │   ├── repository.cpp/h # 备份仓库管理
    │   └── file_utils.cpp/h # 文件操作工具
    ├── metadata/           # 元数据支持
    │   ├── metadata.cpp/h  # 元数据类（mode, mtime, uid/gid预留）
    │   └── filesystem.cpp/h # 文件系统类型识别
    ├── filters/            # 过滤器模块
    │   ├── filter_base.cpp/h # 过滤器基类
    │   └── path_filter.cpp/h # 路径过滤器（已实现）
    └── storage/            # 存储扩展接口
        ├── pack_store.cpp/h # 打包接口（预留）
        ├── compressor.cpp/h # 压缩接口（预留）
        └── encryptor.cpp/h  # 加密接口（预留）
```

## 备份仓库结构

备份仓库采用以下目录结构：

```
<仓库路径>/
├── data/              # 文件数据存储目录
│   └── <相对路径>/    # 按原目录结构存储文件
└── index.txt          # 文件索引和元数据
```

`index.txt` 格式：每行一个文件记录，格式为：
```
<相对路径>\t<mode>:<mtime>:<uid>:<gid>:<is_symlink>:<symlink_target>
```

## 设计说明

### 架构设计

- **核心与CLI解耦**: 核心功能（Backup/Restore/Repository）独立于CLI，便于未来扩展GUI
- **扩展接口预留**: 通过抽象基类预留扩展点（FilterBase, PackStore, Compressor, Encryptor）
- **无第三方依赖**: 仅使用 C++17 标准库和 Linux 系统调用

### 元数据支持

- **已实现**: mode（文件权限）、mtime（修改时间）
- **预留**: uid（用户ID）、gid（组ID） - 需要 root 权限，接口已预留

### 符号链接处理

- 备份时保存符号链接目标路径
- 还原时重新创建符号链接
- 元数据记录符号链接类型

### 路径过滤器

- 支持多个 include/exclude 规则
- 当前实现为简单前缀匹配
- TODO: 可扩展为 glob 或正则表达式匹配

## 限制与注意事项

1. **仅限 Linux**: 使用了 Linux 特定的系统调用（stat, chmod, utimensat 等）
2. **无第三方库**: 压缩、加密等功能未实现，仅预留接口
3. **权限限制**: uid/gid 恢复需要 root 权限，当前仅保存不恢复
4. **路径过滤**: 当前为简单实现，复杂模式匹配需扩展

## 扩展开发指南

### 添加新的过滤器类型

1. 继承 `FilterBase` 类
2. 实现 `shouldInclude()` 方法
3. 在 `FilterBase::Type` 枚举中添加新类型

### 实现压缩功能

1. 实现 `Compressor` 接口
2. 在 `Repository::storeFile()` 中集成压缩
3. 注意：不能依赖第三方库，需自行实现算法

### 实现打包功能

1. 实现 `PackStore` 接口
2. 定义打包数据格式
3. 在仓库存储层集成打包功能

## 许可证

本项目为软件工程实验作业，仅供学习和参考。
