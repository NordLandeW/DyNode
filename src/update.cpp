
#include "update.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "utils.h"

// 核心函数
double executeCmdScript(const std::wstring& _workDir) {
    const std::wstring workDir = _workDir;
    std::wstring script =
        LR"(
    @echo off
    chcp 65001 >nul
    echo Starting update process...
    set workDir=)" +
        workDir +
        LR"(
    echo Working directory is set to %workDir%
    :waitLoop
    tasklist | find /i "DyNode.exe" >nul
    if %ERRORLEVEL% equ 0 (
        echo DyNode.exe is still running, waiting...
        timeout /t 5 /nobreak >nul
        goto waitLoop
    )
    echo DyNode.exe is not running, proceeding with file operations...
    xcopy /e /i /y "%workDir%tmp\\" "%workDir%"
    if %ERRORLEVEL% neq 0 (
        echo Error copying files from tmp directory
        pause
        exit /b 1
    )
    echo Files copied successfully
    rmdir /s /q "%workDir%tmp"
    if %ERRORLEVEL% neq 0 (
        echo Error deleting tmp directory
        pause
        exit /b 1
    )
    echo tmp directory deleted successfully
    del "%workDir%update.zip"
    if %ERRORLEVEL% neq 0 (
        echo Error deleting update.zip
        pause
        exit /b 1
    )
    echo update.zip deleted successfully
    echo Update script completed successfully
    echo Exiting in 2 seconds...
    timeout /t 2 /nobreak >nul
    exit
    )";

    std::wstring filename = workDir + L"update_script.bat";

    // 将脚本转换为 UTF-8 并写入文件
    std::string utf8Script = wstringToUtf8(script);

    std::ofstream file(filename);
    if (file.is_open()) {
        file << utf8Script;
        file.close();
    } else {
        std::wcerr << L"Unable to create script file" << std::endl;
        return -1;
    }

    // 执行脚本
    std::wstring command = L"cmd /c start \"Updater\" \"" + filename + L"\"";
    int result = _wsystem(command.c_str());
    if (result != 0) {
        std::wcerr << L"Failed to execute script" << std::endl;
        return -2;
    }
    return 0;
}

namespace fs = std::filesystem;
double cleanupTempFiles(const std::string& workDir) {
    try {
        for (const auto& entry : fs::directory_iterator(workDir)) {
            if (entry.path().extension() == ".zip" &&
                entry.path().filename().string().find("DyNode") == 0) {
                fs::remove(entry.path());
                std::cout << "Deleted: " << entry.path() << std::endl;
            }
        }

        fs::path tmpDir = workDir + "tmp";
        if (fs::exists(tmpDir) && fs::is_directory(tmpDir)) {
            fs::remove_all(tmpDir);
            std::cout << "Deleted directory: " << tmpDir << std::endl;
        }

        fs::path batFile = workDir + "update_script.bat";
        if (fs::exists(batFile)) {
            fs::remove(batFile);
            std::cout << "Deleted: " << batFile << std::endl;
        }

        return 0.0;
    } catch (const fs::filesystem_error& e) {
        std::cout << "Filesystem error: " << e.what() << std::endl;
        return -1.0;
    } catch (const std::exception& e) {
        std::cout << "General error: " << e.what() << std::endl;
        return -1.0;
    }
}

DYCORE_API double DyCore_update(char* workDir) {
    return executeCmdScript(s2ws(workDir));
}

DYCORE_API double DyCore_cleanup_tmpfiles(char* workDir) {
    return cleanupTempFiles(workDir);
}

DYCORE_API const char* DyCore_get_file_modification_time(char* filePath) {
    thread_local static std::string ret;
    ret = get_file_modification_time(filePath);

    return ret.c_str();
}