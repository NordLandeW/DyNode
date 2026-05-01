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

echo --- Entering DyCore directory...
cd /d "%YYprojectDir%\DyCore"
if not exist "lib\sentry\bin\sentry.dll" (
    goto build_sentry
)
if not exist "lib\sentry\bin\crashpad_handler.exe" (
    goto build_sentry
)
if not exist "lib\sentry\lib\sentry.lib" (
    goto build_sentry
)
if not exist "lib\sentry\include\sentry.h" (
    goto build_sentry
)

echo --- Sentry dependency already exists.
goto build_dycore

:build_sentry
    echo --- Sentry dependency not found or incomplete.
    echo --- Running build_sentry.bat...

    call "build_sentry.bat"
    if %errorlevel% neq 0 (
        echo [ERROR] Sentry build failed.
        exit /b 1
    )

    if not exist "lib\sentry\bin\sentry.dll" goto sentry_missing
    if not exist "lib\sentry\bin\crashpad_handler.exe" goto sentry_missing
    if not exist "lib\sentry\lib\sentry.lib" goto sentry_missing
    if not exist "lib\sentry\include\sentry.h" goto sentry_missing

    echo --- Sentry dependency is ready.
    goto build_dycore

:sentry_missing
    echo [ERROR] Sentry build finished, but required files are still missing.
    exit /b 1

:build_dycore
:: Set build mode here: "release" or "debug"
set BUILD_MODE=release

set CMAKE_PRESET=x64-%BUILD_MODE%
set BUILD_DIR=out/build/x64-%BUILD_MODE%

echo --- Building DyCore (%BUILD_MODE%)...

:: Navigate to the DyCore directory.
cd /d "%YYprojectDir%/DyCore"

:: Configure and build the project.
cmake --preset=%CMAKE_PRESET%
cmake --build %BUILD_DIR%

echo --- DyCore build finished.
