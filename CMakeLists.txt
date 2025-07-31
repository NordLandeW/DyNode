cmake_minimum_required (VERSION 3.14)

project ("DyCore")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DYCORE_SOURCES
    src/DyCore.cpp
    src/delaunator.cpp
    src/utils.cpp
    src/update.cpp
    src/async.cpp
    src/note.cpp
    src/project.cpp
    src/compress.cpp
    src/test.cpp
)
add_library (DyCore SHARED ${DYCORE_SOURCES})

target_compile_definitions(DyCore PRIVATE 
    $<$<CONFIG:RELEASE>:NDEBUG>
    $<$<CONFIG:RELEASE>:_CRT_SECURE_NO_WARNINGS>
)

target_compile_options(DyCore PRIVATE 
    $<$<CONFIG:RELEASE>:-O3>
)

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