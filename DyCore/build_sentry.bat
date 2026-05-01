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

set "TMP_DIR=%~dp0tmp"
set "WORK_DIR=%TMP_DIR%\build_sentry"
set "DEST_DIR=%~dp0lib\sentry"
if not defined SENTRY_BUILD_JOBS (
    set "SENTRY_BUILD_JOBS=%NUMBER_OF_PROCESSORS%"
)
set "_MSPDBSRV_ENDPOINT_=DyNodeSentry_%RANDOM%_%RANDOM%"

if not exist "%TMP_DIR%" (
    mkdir "%TMP_DIR%"
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to create temporary directory: %TMP_DIR%
        exit /b 1
    )
)
if not exist "%WORK_DIR%" (
    mkdir "%WORK_DIR%"
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to create working directory: %WORK_DIR%
        exit /b 1
    )
)
cd /d "%WORK_DIR%"

if exist ".git\" (
    echo [INFO] Reusing existing Sentry Native workspace...
    git reset --hard >nul
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to reset existing workspace.
        goto fail
    )
    git clean -fdx >nul
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to clean existing workspace files.
        goto fail
    )
    git fetch --depth 1 origin tag 0.13.1
    if %errorlevel% neq 0 (
        echo [ERROR] Git fetch failed.
        goto fail
    )
    git checkout --force 0.13.1
    if %errorlevel% neq 0 (
        echo [ERROR] Git checkout failed.
        goto fail
    )
) else (
    echo [INFO] Preparing Sentry Native workspace...
    call :clean_work_dir_files
    if %errorlevel% neq 0 (
        goto fail
    )
    echo [INFO] Cloning Sentry Native 0.13.1...
    git clone --depth 1 --branch 0.13.1 https://github.com/getsentry/sentry-native.git .
    if %errorlevel% neq 0 (
        echo [ERROR] Git clone failed.
        goto fail
    )
)

echo [INFO] Updating submodules...
git submodule update --init --recursive
if %errorlevel% neq 0 (
    echo [ERROR] Submodule update failed.
    goto fail
)

echo [INFO] Configuring CMake...
cmake -B build ^
    -DCMAKE_C_FLAGS="/MP" ^
    -DCMAKE_CXX_FLAGS="/MP" ^
    -DSENTRY_BACKEND=crashpad ^
    -DSENTRY_BUILD_TESTS=OFF ^
    -DSENTRY_BUILD_EXAMPLES=OFF ^
    -DSENTRY_BUILD_SHARED_LIBS=ON
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    goto fail
)

echo [INFO] Building Sentry (RelWithDebInfo) with %SENTRY_BUILD_JOBS% parallel jobs...
cmake --build build --config RelWithDebInfo --parallel %SENTRY_BUILD_JOBS%
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    goto fail
)

echo [INFO] Installing artifacts locally...
cmake --install build --prefix install --config RelWithDebInfo
if %errorlevel% neq 0 (
    echo [ERROR] Installation failed.
    goto fail
)

echo [INFO] Deploying to %DEST_DIR%...
if exist "%DEST_DIR%" rmdir /s /q "%DEST_DIR%"
mkdir "%DEST_DIR%"
xcopy /E /I /Y "install\*" "%DEST_DIR%\" >nul
if %errorlevel% neq 0 (
    echo [ERROR] Failed to deploy Sentry artifacts.
    goto fail
)

cd /d "%~dp0"

echo [SUCCESS] Sentry Native deployed to lib\sentry\
echo [INFO] Build workspace retained for reuse: %WORK_DIR%
exit /b 0

:fail
set "BUILD_ERROR=%errorlevel%"
cd /d "%~dp0"
echo [INFO] Build workspace retained for inspection: %WORK_DIR%
exit /b %BUILD_ERROR%

:clean_work_dir_files
attrib -R -H -S "%WORK_DIR%\*" /S /D >nul 2>nul
del /F /Q "%WORK_DIR%\*" >nul 2>nul
for /D %%d in ("%WORK_DIR%\*") do rmdir /S /Q "%%d" >nul 2>nul
dir /A /B "%WORK_DIR%" | findstr . >nul
if %errorlevel% equ 0 (
    echo [ERROR] Failed to clean working directory contents: %WORK_DIR%
    exit /b 1
)
exit /b 0
