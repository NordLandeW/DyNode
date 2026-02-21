cmake_minimum_required(VERSION 3.28)

project("DyCore")

include(CTest)

# Global and compiler settings

if(MSVC OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    message(STATUS "Compiler is MSVC or Clang-cl, applying static linking runtime lib option")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Third-party dependencies

file(GLOB ZSTD_SOURCES "lib/zstd/*/*.c")
add_library(zstd STATIC ${ZSTD_SOURCES})
target_include_directories(zstd PUBLIC "lib/zstd")

file(GLOB LUA_SOURCES "lib/lua/*.c")
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC "lib/lua")

add_library(pugixml STATIC lib/pugixml/pugixml.cpp)
target_include_directories(pugixml PUBLIC "lib")

add_library(delaunator STATIC lib/delaunator.cpp)
target_include_directories(delaunator PUBLIC "lib")

add_library(xxhash STATIC lib/xxhash/xxhash.c)
target_include_directories(xxhash PUBLIC "lib/xxhash")

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

set(DOCTEST_NO_INSTALL ON CACHE BOOL "Skip doctest install when embedded in DyCore" FORCE)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.12
)
FetchContent_MakeAvailable(doctest)

# Version metadata and generated headers

find_package(Git REQUIRED)

execute_process(COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_EXACT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND ${GIT_EXECUTABLE} status --porcelain WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_DIRTY_STATUS ERROR_QUIET)

if(GIT_EXACT_TAG)
    set(DYNODE_BUILD_TYPE "RELEASE")
    set(DYNODE_VERSION ${GIT_EXACT_TAG})
    if(GIT_DIRTY_STATUS)
        message(WARNING "Building a release version (${DYNODE_VERSION}) from a dirty repository!")
        set(DYNODE_VERSION "${DYNODE_VERSION} (dirty)")
    endif()
else()
    set(DYNODE_BUILD_TYPE "DEV")
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_LATEST_TAG OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-list ${GIT_LATEST_TAG}..HEAD --count WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_COMMITS_SINCE_TAG OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    if(GIT_LATEST_TAG)
        set(DYNODE_VERSION "${GIT_LATEST_TAG}-${GIT_COMMITS_SINCE_TAG} (${GIT_COMMIT_HASH})")
    else()
        set(DYNODE_VERSION "dev (${GIT_COMMIT_HASH})")
    endif()

    if(GIT_DIRTY_STATUS)
        set(DYNODE_VERSION "${DYNODE_VERSION} (dirty)")
    endif()
endif()

message(STATUS "Build Type: ${DYNODE_BUILD_TYPE}")
message(STATUS "DyNode Version: ${DYNODE_VERSION}")

set(VERSION_HEADER "${CMAKE_BINARY_DIR}/generated/version.h")
configure_file(src/version.h.in ${VERSION_HEADER})

message(STATUS "Checking for Google API secrets in environment variables...")
set(GOOG_API_SECRET "$ENV{GOOG_API_SECRET}")
set(GOOG_MEASUREMENT_ID "$ENV{GOOG_MEASUREMENT_ID}")

set(CONFIG_HEADER "${CMAKE_BINARY_DIR}/generated/config.h")
configure_file(src/config.h.in ${CONFIG_HEADER})

# Shared target settings

set(DYCORE_COMMON_INCLUDE_DIRS
    src/
    src/note
    src/project
    src/extension
    src/timing
    src/utils
    src/render
    src/video
    lib/
    lib/pugixml
    lib/sol
    res/
    ${CMAKE_BINARY_DIR}/generated
)

if(MSVC)
    message(STATUS "Compiler is MSVC or Clang-cl, applying MSVC-specific options")
    set(DYCORE_RELEASE_OPT /O2)
else()
    message(STATUS "Compiler is GCC/Clang (or other), applying GNU-style options")
    set(DYCORE_RELEASE_OPT -O3)
endif()

function(dycore_apply_common_target_settings target)
    target_include_directories(${target} PRIVATE ${DYCORE_COMMON_INCLUDE_DIRS})

    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:RELEASE>:NDEBUG>
        $<$<CONFIG:RELEASE>:_CRT_SECURE_NO_WARNINGS>
    )

    target_compile_options(${target} PRIVATE
        $<$<CONFIG:RELEASE>:${DYCORE_RELEASE_OPT}>
    )

    target_link_libraries(${target} PRIVATE 
        zstd lua pugixml delaunator glm::glm xxhash
    )
endfunction()

# Core targets

file(GLOB_RECURSE DYCORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(DyCore_objs OBJECT ${DYCORE_SOURCES})
dycore_apply_common_target_settings(DyCore_objs)

# Required for linking object files into a shared library.
set_target_properties(DyCore_objs PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Ensure export macros are visible while compiling object files.
target_compile_definitions(DyCore_objs PRIVATE DyCore_EXPORTS)


if(WIN32)
    set(DYCORE_RESOURCES res/resource.rc)
endif()

add_library(DyCore SHARED $<TARGET_OBJECTS:DyCore_objs> ${DYCORE_RESOURCES})
dycore_apply_common_target_settings(DyCore) 

# Install and post-build steps

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

# Unit tests

file(GLOB DYCORE_TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")

add_executable(DyCore_tests
    $<TARGET_OBJECTS:DyCore_objs>
    ${DYCORE_TEST_SOURCES}
)

dycore_apply_common_target_settings(DyCore_tests)
target_link_libraries(DyCore_tests PRIVATE doctest::doctest)

add_custom_target(run_tests
    COMMAND DyCore_tests --order-by=file --duration=true
    DEPENDS DyCore_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running DyCore doctest suite"
)

add_test(
    NAME DyCore.doctest
    COMMAND $<TARGET_FILE:DyCore_tests> --order-by=file --duration=true
)
