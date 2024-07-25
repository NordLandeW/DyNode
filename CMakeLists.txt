cmake_minimum_required (VERSION 3.8)

project ("DyCore")

# 添加生成 compile_commands.json 的设置
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置Release模式下的优化选项
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

# 将源代码添加到此项目的可执行文件
add_library (DyCore SHARED "DyCore.cpp" "delaunator.cpp")

# 添加依赖库
include_directories ("${CMAKE_SOURCE_DIR}/lib/zstd")
include_directories ("${CMAKE_SOURCE_DIR}/lib/lua")
include_directories ("${CMAKE_SOURCE_DIR}/lib")
file(GLOB ZSTD_SOURCES "${CMAKE_SOURCE_DIR}/lib/zstd/*/*.c")
file(GLOB LUA_SOURCES "${CMAKE_SOURCE_DIR}/lib/lua/*.c")

add_library(zstd STATIC ${ZSTD_SOURCES})
add_library(lua STATIC ${LUA_SOURCES})

# 链接依赖库
target_link_libraries(DyCore PRIVATE zstd)
target_link_libraries(DyCore PRIVATE lua)

# 设置C++标准
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DyCore PROPERTY CXX_STANDARD 20)
endif()

# 安装指令
install(TARGETS DyCore
    RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/../extensions/DyCore"
)

# 输出安装路径确认
message(STATUS "DyCore will be installed to ${CMAKE_SOURCE_DIR}/../extensions/DyCore")
