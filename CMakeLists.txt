cmake_minimum_required (VERSION 3.14)

project ("DyCore")

if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

## Dependencies
# --- zstd ---
file(GLOB ZSTD_SOURCES "lib/zstd/*/*.c")
add_library(zstd STATIC ${ZSTD_SOURCES})
target_include_directories(zstd PUBLIC "lib/zstd")

# --- lua ---
file(GLOB LUA_SOURCES "lib/lua/*.c")
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC "lib/lua")

# --- pugixml ---
add_library(pugixml STATIC lib/pugixml.cpp)
target_include_directories(pugixml PUBLIC "lib")

# --- delaunator ---
add_library(delaunator STATIC lib/delaunator.cpp)
target_include_directories(delaunator PUBLIC "lib")

## DyCore

set(DYCORE_SOURCES
    src/DyCore.cpp
    src/delaunator.cpp
    src/utils.cpp
    src/update.cpp
    src/async.cpp
    src/note/note.cpp
    src/note/notePoolManager.cpp
    src/note/noteStat.cpp
    src/project/project.cpp
    src/compress.cpp
    src/test.cpp
    src/singletons.cpp
)
add_library (DyCore SHARED ${DYCORE_SOURCES})
target_include_directories(DyCore PRIVATE
    src
    src/note
    src/project
)

target_compile_definitions(DyCore PRIVATE 
    $<$<CONFIG:RELEASE>:NDEBUG>
    $<$<CONFIG:RELEASE>:_CRT_SECURE_NO_WARNINGS>
)

if(MSVC)
    target_compile_options(DyCore PRIVATE
        $<$<CONFIG:RELEASE>:/O2>
        $<$<CONFIG:RELEASE>:/arch:AVX2>
    )
else()
    target_compile_options(DyCore PRIVATE
        $<$<CONFIG:RELEASE>:-O3>
        $<$<CONFIG:RELEASE>:-mavx2>
    )
endif()

target_link_libraries(DyCore PRIVATE zstd)
target_link_libraries(DyCore PRIVATE lua)
target_link_libraries(DyCore PRIVATE pugixml)
target_link_libraries(DyCore PRIVATE delaunator)

install(TARGETS DyCore
    RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/../extensions/DyCore"
)

add_custom_command(TARGET DyCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install . --config $<CONFIG>
    COMMENT "Automatically installing DyCore after build..."
)

message(STATUS "DyCore will be installed to ${CMAKE_SOURCE_DIR}/../extensions/DyCore")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(COMPILE_COMMANDS_SOURCE "${CMAKE_BINARY_DIR}/compile_commands.json")
  set(COMPILE_COMMANDS_LINK "${CMAKE_SOURCE_DIR}/compile_commands.json")

  add_custom_command(
      OUTPUT ${COMPILE_COMMANDS_LINK}
      COMMAND ${CMAKE_COMMAND} -E create_symlink ${COMPILE_COMMANDS_SOURCE} ${COMPILE_COMMANDS_LINK}
      DEPENDS ${COMPILE_COMMANDS_SOURCE}
      COMMENT "Creating symlink for compile_commands.json in project root"
  )
  
  add_custom_target(symlink_compile_commands ALL
      DEPENDS ${COMPILE_COMMANDS_LINK}
  )
endif()