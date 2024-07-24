// DyCore.cpp: 定义应用程序的入口点。
//

#include "DyCore.h"

const int RETURN_BUFFER_SIZE = 50 * 1024 * 1024;
char _return_buffer[RETURN_BUFFER_SIZE];

DYCORE_API const char* DyCore_init() {

    return "success";
}

DYCORE_API const char* DyCore_delaunator(char* in_struct) {
    json j = json::parse(in_struct);

    if (!j.is_array())
        return "Error!in_struct must be an array.";

    auto coords = j.get<std::vector<double>>();

    delaunator::Delaunator d(coords);

    json r = json::array();

    for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
        r.push_back(
            {{"p1",
              {d.coords[2 * d.triangles[i]], d.coords[2 * d.triangles[i] + 1]}},
             {"p2",
              {d.coords[2 * d.triangles[i + 1]],
               d.coords[2 * d.triangles[i + 1] + 1]}},
             {"p3",
              {d.coords[2 * d.triangles[i + 2]],
               d.coords[2 * d.triangles[i + 2] + 1]}}});
    }

    strcpy_s(_return_buffer, r.dump().c_str());

    return _return_buffer;
}

double executeCmdScript(const std::string& workDir) {
    std::string script = R"(
    @echo off
    echo Starting update process...
    set workDir=)" + workDir +
                         R"(
    echo Working directory is set to %workDir%
    :waitLoop
    tasklist | find /i "DyNode.exe" >nul
    if %ERRORLEVEL% equ 0 (
        echo DyNode.exe is still running, waiting...
        timeout /t 5 /nobreak >nul
        goto waitLoop
    )
    echo DyNode.exe is not running, proceeding with file operations...
    xcopy /e /i /y "%workDir%tmp\" "%workDir%"
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

    std::string filename = workDir + "update_script.bat";
    std::ofstream file(filename);
    if (file.is_open()) {
        file << script;
        file.close();
    } else {
        std::cerr << "Unable to create script file" << std::endl;
        return -1;
    }

    std::string command = "cmd /c start " + filename;
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Failed to execute script" << std::endl;
        return -2;
    }
    return 0;
}

DYCORE_API double DyCore_update(char* workDir) {
    return executeCmdScript(workDir);
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
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return -1.0;
    } catch (const std::exception& e) {
        std::cerr << "General error: " << e.what() << std::endl;
        return -1.0;
    }
}

DYCORE_API double DyCore_cleanup_tmpfiles(char* workDir) {
    return cleanupTempFiles(workDir);
}

DYCORE_API double DyCore_compress_string(const char* str, char* targetBuffer,
                                         double compressionLevel) {
    size_t fSize = strlen(str);
    size_t cBuffSize = ZSTD_compressBound(fSize);

    char* const fBuff = new char[fSize];
    char* const cBuff = new char[cBuffSize];

    memcpy(fBuff, str, fSize);

    std::cout << "[DyCore] Start compressing..." << std::endl;

    size_t const cSize =
        ZSTD_compress(cBuff, cBuffSize, fBuff, fSize, (int)compressionLevel);

    std::cout << "[DyCore] Finish compressing, checking..." << std::endl;

    try {
        CHECK_ZSTD(cSize);
    } catch (...) {
        delete[] cBuff;
        delete[] fBuff;
        return -1;
    };

    std::cout << "[DyCore] No error found. Success. " << fSize << "->" << cSize
              << std::endl;

    // Success
    memcpy(targetBuffer, cBuff, cSize);

    delete[] cBuff;
    delete[] fBuff;

    return (double)cSize;
}

bool check_compressed(const char* str, double _sSize) {
    size_t sSize = (size_t)_sSize;
    unsigned long long const rSize = ZSTD_getFrameContentSize(str, sSize);
    return rSize != ZSTD_CONTENTSIZE_ERROR && rSize != ZSTD_CONTENTSIZE_UNKNOWN;
}

DYCORE_API double DyCore_is_compressed(const char* str, double _sSize) {
    size_t sSize = (size_t)_sSize;
    return check_compressed(str, sSize) ? 0.0 : -1.0;
}

DYCORE_API const char* DyCore_decompress_string(const char* str,
                                                double _sSize) {
    size_t sSize = (size_t)_sSize;
    if (!check_compressed(str, sSize)) {
        std::cout << "[DyCore] Not a zstd file!" << std::endl;

        return "failed";
    }

    std::cout << "[DyCore] Start decompressing..." << std::endl;

    size_t const rSize = ZSTD_getFrameContentSize(str, sSize);
    char* const rBuff = new char[rSize];
    size_t const dSize = ZSTD_decompress(rBuff, rSize, str, sSize);

    if (dSize != rSize)
        return "failed";

    // Success
    memcpy(_return_buffer, rBuff, rSize);

    std::cout << "[DyCore] No error found. Sucess." << std::endl;

    delete[] rBuff;

    return _return_buffer;
}

DYCORE_API double DyCore_buffer_copy(void* dst, void* src, double size) {
    memcpy(dst, src, (size_t)size);
    return 0;
}