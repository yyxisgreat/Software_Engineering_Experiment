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
- 📋 **打包/解包**: 将所有备份文件拼接为一个大文件保存
- 📋 **压缩/解压**: 通过文件压缩节省备份文件的存储空间
- 📋 **加密/解密**: 由用户指定密码，将所有备份文件均加密保存
- 🖥️ **图形界面（GUI）**: GUI 接口已预留，支持进度回调和状态更新（见下文）

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
    ├── storage/            # 存储扩展接口
    │   ├── pack_store.cpp/h # 打包接口
    │   ├── compressor.cpp/h # 压缩接口
    │   └── encryptor.cpp/h  # 加密接口
    └── gui/                # GUI 接口模块
        ├── gui_interface.h  # GUI 进度回调和操作接口（已定义）
        └── gui_interface.cpp # GUI 接口实现（已实现）
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

### 实现图形界面（GUI）

项目已提供 GUI 接口定义（`src/gui/gui_interface.h`），支持以下功能：

#### GUI 接口说明

**1. 进度回调接口 (`ProgressCallback`)**

GUI 应用需要实现 `ProgressCallback` 接口来接收操作进度更新：

- `onStart()`: 操作开始时调用，提供总文件数和操作名称
- `onProgress()`: 每个文件处理时调用，提供当前文件路径、进度百分比等信息
- `onFileSuccess()`: 文件处理成功时调用
- `onFileError()`: 文件处理失败时调用，提供错误信息
- `onFileSkipped()`: 文件被跳过时调用，提供跳过原因
- `onComplete()`: 操作完成时调用，提供统计信息
- `shouldCancel()`: 检查是否应该取消操作（支持用户取消）

**2. GUI 操作接口 (`GuiOperations`)**

提供 GUI 友好的高级操作接口：

- `backupWithProgress()`: 执行备份操作，支持进度回调
- `restoreWithProgress()`: 执行还原操作，支持进度回调
- `listBackupFiles()`: 列出备份仓库中的文件
- `validateRepository()`: 验证备份仓库是否有效

#### GUI 实现步骤

**方案一：使用 Qt（推荐）**

1. **安装 Qt 开发库**：
   ```bash
   # Ubuntu/Debian
   sudo apt-get install qt6-base-dev qt6-tools-dev
   
   # 或使用 Qt5
   sudo apt-get install qt5-default qtbase5-dev
   ```

2. **创建 GUI 主窗口**：
   - 使用 Qt Designer 设计界面（.ui 文件）
   - 或使用代码创建窗口（QMainWindow/QWidget）

3. **实现进度回调类**：
   ```cpp
   #include "gui/gui_interface.h"
   #include <QProgressBar>
   #include <QLabel>
   #include <QTextEdit>
   
   class QtProgressCallback : public backuprestore::ProgressCallback {
   public:
       QtProgressCallback(QProgressBar* progressBar, 
                         QLabel* statusLabel, 
                         QTextEdit* logText)
           : progress_bar_(progressBar), status_label_(statusLabel), log_text_(logText) {}
       
       void onStart(std::size_t total_files, const std::string& operation_name) override {
           progress_bar_->setMaximum(total_files);
           progress_bar_->setValue(0);
           status_label_->setText(QString::fromStdString(operation_name + "开始..."));
       }
       
       void onProgress(const std::filesystem::path& current_file,
                      std::size_t current_index,
                      std::size_t total_files,
                      double percentage) override {
           progress_bar_->setValue(current_index);
           status_label_->setText(QString::fromStdString("处理: " + current_file.string()));
           // 更新日志
           log_text_->append(QString::fromStdString(
               "[" + std::to_string(current_index) + "/" + std::to_string(total_files) + "] " +
               current_file.string()));
       }
       
       // ... 实现其他回调方法
       
       bool shouldCancel() const override {
           // 检查用户是否点击了取消按钮
           return cancelled_;
       }
       
   private:
       QProgressBar* progress_bar_;
       QLabel* status_label_;
       QTextEdit* log_text_;
       bool cancelled_ = false;
   };
   ```

4. **在主窗口中调用操作**：
   ```cpp
   void MainWindow::onBackupButtonClicked() {
       QString sourceDir = ui->sourceDirEdit->text();
       QString repoDir = ui->repoDirEdit->text();
       
       QtProgressCallback callback(ui->progressBar, ui->statusLabel, ui->logText);
       
       // 在后台线程中执行备份（避免阻塞UI）
       QThread* thread = QThread::create([=, &callback]() {
           backuprestore::GuiOperations::backupWithProgress(
               sourceDir.toStdString(),
               repoDir.toStdString(),
               {},  // include_paths
               {},  // exclude_paths
               &callback
           );
       });
       thread->start();
   }
   ```

5. **更新 CMakeLists.txt**：
   ```cmake
   find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
   # 或使用 Qt5: find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
   
   add_executable(gui-app
       src/gui/main_window.cpp
       src/gui/main_window.h
       src/gui/progress_callback.cpp
   )
   
   target_link_libraries(gui-app Qt6::Core Qt6::Widgets)
   # 或: target_link_libraries(gui-app Qt5::Core Qt5::Widgets)
   ```

**方案二：使用 GTK+**

1. **安装 GTK+ 开发库**：
   ```bash
   sudo apt-get install libgtk-3-dev
   ```

2. **实现 GTK+ 进度回调**：
   ```cpp
   class GtkProgressCallback : public backuprestore::ProgressCallback {
   public:
       GtkProgressCallback(GtkProgressBar* progress_bar, 
                          GtkLabel* status_label)
           : progress_bar_(progress_bar), status_label_(status_label) {}
       
       void onProgress(...) override {
           gtk_progress_bar_set_fraction(progress_bar_, percentage / 100.0);
           gtk_label_set_text(status_label_, current_file.c_str());
       }
       // ... 其他方法
   };
   ```

**方案三：使用 Web 技术（Electron + C++ 后端）**

1. 使用 C++ 实现 HTTP 服务器（如使用 Crow、cpp-httplib 等）
2. 前端使用 HTML/CSS/JavaScript 构建界面
3. 通过 HTTP API 调用 `GuiOperations` 接口
4. 使用 WebSocket 或 Server-Sent Events 推送进度更新

#### GUI 界面建议功能

- **备份界面**：
  - 源目录选择（文件浏览器）
  - 备份仓库路径选择
  - 包含/排除路径配置（列表编辑）
  - 开始备份按钮
  - 进度条显示
  - 当前处理文件显示
  - 操作日志显示
  - 取消按钮

- **还原界面**：
  - 备份仓库路径选择
  - 目标目录选择
  - 开始还原按钮
  - 进度条显示
  - 当前处理文件显示
  - 操作日志显示
  - 取消按钮

- **仓库管理界面**：
  - 备份仓库列表
  - 仓库文件浏览
  - 仓库验证
  - 删除仓库功能

#### 注意事项

1. **线程安全**：备份/还原操作应在后台线程执行，避免阻塞 UI 线程
2. **进度更新**：使用信号槽机制（Qt）或主线程回调（GTK+）更新 UI
3. **错误处理**：在回调中捕获并显示错误信息
4. **取消操作**：实现 `shouldCancel()` 方法，定期检查取消标志
5. **依赖管理**：GUI 库依赖需要在 CMakeLists.txt 中正确配置

## 许可证

本项目为软件工程实验作业，仅供学习和参考。
