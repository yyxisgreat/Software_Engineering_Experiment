cmake_minimum_required(VERSION 3.10)
project(BackupRestore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG")

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)

# 源文件
set(CORE_SOURCES
    src/core/backup.cpp
    src/core/restore.cpp
    src/core/repository.cpp
    src/core/file_utils.cpp
)

set(METADATA_SOURCES
    src/metadata/metadata.cpp
    src/metadata/filesystem.cpp
)

set(FILTER_SOURCES
    src/filters/path_filter.cpp
    src/filters/filter_base.cpp
)

set(STORAGE_SOURCES
    src/storage/pack_store.cpp
    src/storage/compressor.cpp
    src/storage/encryptor.cpp

    # ===== 新增：package 导入导出功能（打包/压缩/加密）=====
    src/storage/package/compress_rle.cpp
    src/storage/package/encrypt_xor.cpp
    src/storage/package/encrypt_rc4.cpp
    src/storage/package/pack_header.cpp
    src/storage/package/pack_toc.cpp
    src/storage/package/package_export.cpp
)

set(SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${METADATA_SOURCES}
    ${FILTER_SOURCES}
    ${STORAGE_SOURCES}
)

# 可执行文件
add_executable(backup-restore ${SOURCES})

# 链接库（仅使用系统库）
target_link_libraries(backup-restore)

if(WIN32)
    target_link_libraries(backup-restore kernel32 user32)
endif()
