cmake_minimum_required (VERSION 3.14)

project ("DyCore")

# 添加生成 compile_commands.json 的设置
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置Release模式下的优化选项
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG -D_CRT_SECURE_NO_WARNINGS")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG -D_CRT_SECURE_NO_WARNINGS")

set(DYCORE_SOURCES
    src/DyCore.cpp
    src/delaunator.cpp
    src/utils.cpp
    src/update.cpp
    src/async.cpp
    src/note.cpp
    src/project.cpp
)
add_library (DyCore SHARED ${DYCORE_SOURCES})

# 添加依赖库
include_directories("${CMAKE_SOURCE_DIR}/lib/zstd")
include_directories("${CMAKE_SOURCE_DIR}/lib/lua")
include_directories("${CMAKE_SOURCE_DIR}/lib")
file(GLOB ZSTD_SOURCES "${CMAKE_SOURCE_DIR}/lib/zstd/*/*.c")
file(GLOB LUA_SOURCES "${CMAKE_SOURCE_DIR}/lib/lua/*.c")
file(GLOB PUGIXML_SOURCES "${CMAKE_SOURCE_DIR}/lib/pugixml.cpp")
file(GLOB DELAUNATOR_SOURCES "${CMAKE_SOURCE_DIR}/lib/delaunator.cpp")
add_library(zstd STATIC ${ZSTD_SOURCES})
add_library(lua STATIC ${LUA_SOURCES})
add_library(pugixml STATIC ${PUGIXML_SOURCES})
add_library(delaunator STATIC ${DELAUNATOR_SOURCES})

# 链接依赖库
target_link_libraries(DyCore PRIVATE zstd)
target_link_libraries(DyCore PRIVATE lua)
target_link_libraries(DyCore PRIVATE pugixml)
target_link_libraries(DyCore PRIVATE delaunator)

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
