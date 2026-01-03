# 目录树备份 / 还原软件（Directory Backup & Restore Tool）

一个基于 **C++17** 的 Linux / WSL 目录树备份与还原工具。  
支持 **普通文件、符号链接、命名管道（FIFO）** 的正确备份与还原，支持 **元数据保存** 与 **自定义备份过滤**，并为设备文件、压缩、加密、GUI 等功能预留了清晰的扩展接口。

本项目为软件工程课程实验项目，重点体现 **系统架构设计、文件系统语义建模与可扩展性设计**。

---

## 一、功能特性

### 1. 已实现功能（V2）

#### ✅ 基础能力
- **数据备份**：递归遍历目录树并写入备份仓库  
- **数据还原**：从仓库恢复完整目录结构  
- **Linux / WSL 支持**：基于 POSIX 系统调用实现

#### ✅ 文件类型支持
- **普通文件（Regular）**
  - 备份：文件实体写入仓库 `data/`
  - 还原：复制文件并恢复权限、时间戳
- **符号链接（Symlink）**
  - 备份：**仅记录元数据（不写入 `data/`）**
  - 还原：使用 `symlink()` 重新创建符号链接
- **命名管道（FIFO）**
  - 备份：仅记录元数据
  - 还原：使用 `mkfifo()` 创建管道
- **设备文件 / Socket**
  - 文件类型与主次设备号已记录
  - 还原逻辑预留（需 root 权限，未启用）

#### ✅ 元数据支持（已扩展）
- `mode`（权限）
- `mtime`（修改时间）
- `uid / gid`（用户 / 组 ID，已记录，默认不强制恢复）
- `file_type`（文件类型枚举）
- `symlink_target`（符号链接目标）

#### ✅ 自定义备份（过滤器）
- **路径过滤**（include / exclude）
- **文件类型过滤**（`--type regular | symlink | fifo | device | socket`）
- **文件名过滤**（`--name-contains <substr>`）
- 时间 / 大小 / 用户过滤接口已具备，可扩展

#### ✅ 打包 / 解包 / 压缩 / 加密
- **导出（export）**：将完整仓库目录打包为单文件
- **导入（import）**：将包文件解包恢复为仓库目录
- 支持包中对文件数据进行 **RLE 压缩** 和 **异或 (XOR) / RC4 加密**
- 导出时可指定：是否打包、压缩算法、加密算法、密码；导入时可指定解密密码

#### ✅ 架构与扩展
- 核心逻辑与 CLI / GUI 解耦
- 过滤器链（FilterChain）支持组合扩展
- 压缩 / 加密 / 打包接口已预留可扩展多种算法
- GUI 进度回调接口已定义

---

## 二、编译环境要求

- **操作系统**：Linux / WSL Ubuntu
- **编译器**：GCC 7+ 或 Clang 5+
- **语言标准**：C++17
- **构建工具**：CMake ≥ 3.10
- **依赖库**：无第三方依赖（仅使用标准库与 POSIX）

---

## 三、编译方法

```bash
# 进入项目根目录
mkdir -p build
cd build

# 生成构建文件
cmake ..

# 编译
make -j

# 可执行文件：
./backup-restore
```

---

## 四、使用方法

### 1. 备份目录

```
./backup-restore backup <源目录> <仓库目录> [过滤器选项]
```

示例：

```bash
./backup-restore backup demo_src demo_repo
```

#### 带过滤器的备份

```bash
# 路径过滤
./backup-restore backup /home/user /backup/repo \
  --include /home/user/docs \
  --exclude /home/user/tmp

# 文件类型过滤（仅普通文件）
./backup-restore backup demo_src demo_repo --type regular

# 文件名过滤
./backup-restore backup demo_src demo_repo --name-contains a
```

### 2. 还原目录

```
./backup-restore restore <仓库目录> <目标目录>
```

示例：

```bash
./backup-restore restore demo_repo demo_dst
```

### 3. 导出与导入

```
./backup-restore export <仓库目录> <包文件> [--pack yes/no] [--compress rle] [--encrypt xor|rc4] [--password <pwd>]
./backup-restore import <包文件> <仓库目录> [--password <pwd>]
```

导出示例（打包+RLE压缩+RC4加密）：

```bash
./backup-restore export demo_repo demo_package.dat \
  --pack yes --compress rle --encrypt rc4 --password secret
```

导入示例：

```bash
./backup-restore import demo_package.dat demo_repo --password secret
```

---

## 五、示例演示（推荐测试）

```bash
# 准备测试数据
echo "hello world" > demo_src/a.txt
mkfifo demo_src/myfifo
ln -s a.txt demo_src/link_to_a

# 执行备份
./backup-restore backup demo_src demo_repo

# 查看索引
cat demo_repo/index.txt

# 执行还原
./backup-restore restore demo_repo demo_dst

# 验证结果
ls -l demo_dst
readlink demo_dst/link_to_a

# 过滤示例
./backup-restore backup demo_src demo_repo --type regular
./backup-restore backup demo_src demo_repo --name-contains a

# 导出与导入示例
./backup-restore export demo_repo demo_package.dat \
  --pack yes --compress rle --encrypt rc4 --password secret
rm -rf demo_repo
./backup-restore import demo_package.dat demo_repo --password secret
```

---

## 六、项目结构

```text
Software_Engineering_Experiment/
├── CMakeLists.txt
├── README.md
├── DESIGN.md                 # 设计书（V2）
├── run_tests.sh              # 测试脚本（示例）
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── backup.cpp/h
│   │   ├── restore.cpp/h
│   │   ├── repository.cpp/h
│   │   └── file_utils.cpp/h
│   ├── metadata/
│   │   ├── metadata.cpp/h
│   │   └── filesystem.cpp/h
│   ├── filters/
│   │   ├── filter_base.cpp/h
│   │   ├── filter_chain.cpp/h
│   │   ├── path_filter.cpp/h
│   │   ├── file_type_filter.cpp/h
│   │   ├── name_filter.cpp/h
│   │   ├── time_filter.cpp/h
│   │   ├── size_filter.cpp/h
│   │   └── user_filter.cpp/h
│   ├── storage/
│   │   ├── package/
│   │   │   ├── package_export.cpp/h
│   │   │   ├── pack_header.cpp/h
│   │   │   ├── pack_toc.cpp/h
│   │   │   ├── compress_rle.cpp/h
│   │   │   ├── encrypt_xor.cpp/h
│   │   │   ├── encrypt_rc4.cpp/h
│   │   ├── pack_store.cpp/h      # 打包接口（预留）
│   │   ├── compressor.cpp/h      # 压缩接口（预留）
│   │   └── encryptor.cpp/h       # 加密接口（预留）
│   └── gui/
│       ├── gui_interface.cpp/h
└── test/
    ├── source/                 # 示例测试源目录
    ├── repo/                   # 示例测试仓库目录
    └── target/                 # 示例测试还原目录
```

---

## 七、备份仓库结构

```text
<repo>/
├── data/                 # 仅存放普通文件实体
│   └── <relative_path>
└── index.txt             # 所有文件的元数据索引
```

### index.txt（V2 格式）

```text
<relative_path>\t<mode>:<mtime>:<uid>:<gid>:<file_type>:<dev_major>:<dev_minor>:<is_symlink>:<symlink_target>
```

示例：

```text
a.txt      33188:1766816422:1000:1000:0:0:0:0:
link_to_a  41471:1766816422:1000:1000:2:0:0:1:a.txt
myfifo     4516:1766816422:1000:1000:5:0:0:0:
```