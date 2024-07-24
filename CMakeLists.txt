# CMakeList.txt: DyCore 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("DyCore")

# 添加生成 compile_commands.json 的设置
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置Release模式下的优化选项
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")

# 将源代码添加到此项目的可执行文件。
add_library (DyCore SHARED "DyCore.cpp" "delaunator.cpp")

# Add libraries.
include_directories ("${CMAKE_SOURCE_DIR}/lib/zstd")
include_directories ("${CMAKE_SOURCE_DIR}/lib/lua")
include_directories ("${CMAKE_SOURCE_DIR}/lib")
file(GLOB ZSTD_SOURCES "${CMAKE_SOURCE_DIR}/lib/zstd/*/*.c")
file(GLOB LUA_SOURCES "${CMAKE_SOURCE_DIR}/lib/lua/*.c")

# 添加Zstd库
add_library(zstd STATIC ${ZSTD_SOURCES})
add_library(lua STATIC ${LUA_SOURCES})

# 链接Zstd库
target_link_libraries(DyCore PRIVATE zstd)
target_link_libraries(DyCore PRIVATE lua)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DyCore PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
