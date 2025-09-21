@echo off
:: Build DyCore before running the project.

:: Set build mode here: "release" or "debug"
set BUILD_MODE=release

set CMAKE_PRESET=x64-%BUILD_MODE%
set BUILD_DIR=out/build/x64-%BUILD_MODE%

echo --- Building DyCore (%BUILD_MODE%)...

:: Navigate to the DyCore directory.
cd %YYprojectDir%/DyCore

:: Configure and build the project.
cmake --preset=%CMAKE_PRESET%
cmake --build %BUILD_DIR%

echo --- DyCore build finished.