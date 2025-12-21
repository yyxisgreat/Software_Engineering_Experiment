#!/bin/bash

# 构建脚本

set -e

echo "创建构建目录..."
mkdir -p build
cd build

echo "配置 CMake..."
cmake ..

echo "编译项目..."
make -j$(nproc)

echo "构建完成！可执行文件位于: build/backup-restore"


