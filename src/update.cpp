
#include "update.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "utils.h"

// Executes a batch script to perform application updates.
// The script waits for "DyNode.exe" to close, copies new files,
// cleans up temporary files, and then exits.
//
// @param _workDir The working directory for the update process.
// @return 0 on success, -1 if script creation fails, -2 if execution fails.
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
// Cleans up temporary files created during the update process,
// including zip files, the 'tmp' directory, and the update script itself.
//
// @param workDir The directory to clean up.
// @return 0.0 on success, -1.0 on error.
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

// Initiates the application update process by executing the update script.
//
// @param workDir The working directory.
// @return The result of executeCmdScript.
DYCORE_API double DyCore_update(char* workDir) {
    return executeCmdScript(s2ws(workDir));
}

// Cleans up temporary files from the working directory.
//
// @param workDir The working directory.
// @return The result of cleanupTempFiles.
DYCORE_API double DyCore_cleanup_tmpfiles(char* workDir) {
    return cleanupTempFiles(workDir);
}

// Retrieves the last modification time of a file.
// This is a wrapper around the utility function to be exposed via the API.
//
// @param filePath The path to the file.
// @return The file's modification time as a string.
DYCORE_API const char* DyCore_get_file_modification_time(char* filePath) {
    thread_local static std::string ret;
    ret = get_file_modification_time(filePath);

    return ret.c_str();
}
