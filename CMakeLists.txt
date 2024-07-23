# CMakeList.txt: DyCore 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("DyCore")

# 添加生成 compile_commands.json 的设置
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 将源代码添加到此项目的可执行文件。
add_library (DyCore SHARED "DyCore.cpp" "DyCore.h" "delaunator.cpp" "delaunator.hpp" "json.hpp")

# Add libraries.
include_directories ("${CMAKE_SOURCE_DIR}/lib/zstd")
file(GLOB ZSTD_SOURCES "${CMAKE_SOURCE_DIR}/lib/zstd/*/*.c")

# 添加Zstd库
add_library(zstd STATIC ${ZSTD_SOURCES})

# 链接Zstd库
target_link_libraries(DyCore PRIVATE zstd)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DyCore PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
