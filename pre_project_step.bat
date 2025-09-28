@echo off
:: Exit if running in a CI environment (like GitHub Actions)
if defined CI (
    echo --- Skipping pre_project_step.bat in CI environment.
    exit /b 0
)

:: Build DyCore before running the project.

:: Load environment variables from .env file
for /f "usebackq delims=" %%a in ("%YYprojectDir%/.env") do (
    set "%%a"
)

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