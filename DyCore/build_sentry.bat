@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo [INFO] Initializing Sentry Native build process...

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Visual Studio is required.
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo [ERROR] Visual Studio with C++ desktop development workload not found.
    exit /b 1
)

echo [INFO] Found Visual Studio: %VS_PATH%
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul

set "WORK_DIR=%~dp0tmp\build_sentry"
set "DEST_DIR=%~dp0lib\sentry"

if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%"
mkdir "%WORK_DIR%"
cd /d "%WORK_DIR%"

echo [INFO] Cloning Sentry Native 0.13.1...
git clone --depth 1 --branch 0.13.1 https://github.com/getsentry/sentry-native.git .
if %errorlevel% neq 0 (
    echo [ERROR] Git clone failed.
    exit /b 1
)

echo [INFO] Updating submodules...
git submodule update --init --recursive
if %errorlevel% neq 0 (
    echo [ERROR] Submodule update failed.
    exit /b 1
)

echo [INFO] Configuring CMake...
cmake -B build -DSENTRY_BACKEND=crashpad -DSENTRY_BUILD_TESTS=OFF -DSENTRY_BUILD_EXAMPLES=OFF -DSENTRY_BUILD_SHARED_LIBS=ON
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo [INFO] Building Sentry (RelWithDebInfo)...
cmake --build build --config RelWithDebInfo --parallel
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [INFO] Installing artifacts locally...
cmake --install build --prefix install --config RelWithDebInfo
if %errorlevel% neq 0 (
    echo [ERROR] Installation failed.
    exit /b 1
)

echo [INFO] Deploying to %DEST_DIR%...
if exist "%DEST_DIR%" rmdir /s /q "%DEST_DIR%"
mkdir "%DEST_DIR%"
xcopy /E /I /Y "install\*" "%DEST_DIR%\" >nul

echo [INFO] Cleaning up working directory...
cd /d "%~dp0"
rmdir /s /q "%WORK_DIR%"

echo [SUCCESS] Sentry Native deployed to lib\sentry\
exit /b 0