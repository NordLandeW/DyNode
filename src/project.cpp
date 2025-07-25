
#include "project.h"

#include <zstd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>

#include "api.h"
#include "async.h"
#include "note.h"
#include "utils.h"

using std::string;

DYCORE_API double DyCore_compress_string(const char* str, char* targetBuffer,
                                         double compressionLevel) {
    if (!str || !targetBuffer) {
        print_debug_message("Error: Null pointer passed to compress_string.");
        return -1;
    }
    compressionLevel = std::clamp((int)compressionLevel, 0, 22);
    // Skip the compression if the compression level is 0.
    if ((int)compressionLevel == 0) {
        size_t fSize = strlen(str);
        memcpy(targetBuffer, str, fSize);
        std::cout << "[DyCore] Compression level set to 0. Skip compression."
                  << std::endl;
        return fSize;
    }

    size_t fSize = strlen(str);
    size_t cBuffSize = ZSTD_compressBound(fSize);

    auto fBuff = std::make_unique<char[]>(fSize);
    auto cBuff = std::make_unique<char[]>(cBuffSize);

    memcpy(fBuff.get(), str, fSize);

    std::cout << "[DyCore] Start compressing..." << std::endl;

    size_t const cSize = ZSTD_compress(cBuff.get(), cBuffSize, fBuff.get(),
                                       fSize, (int)compressionLevel);

    std::cout << "[DyCore] Finish compressing, checking..." << std::endl;

    try {
        CHECK_ZSTD(cSize);
    } catch (...) {
        return -1;
    };

    std::cout << "[DyCore] No error found. Success. " << fSize << "->" << cSize
              << "(" << ((double)cSize / fSize * 100.0) << "%)" << std::endl;

    // Success
    memcpy(targetBuffer, cBuff.get(), cSize);

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

string decompress_string(const char* str, double _sSize) {
    size_t sSize = (size_t)_sSize;
    if (!check_compressed(str, sSize)) {
        std::cout << "[DyCore] Not a zstd file!" << std::endl;

        return "failed";
    }

    std::cout << "[DyCore] Start decompressing..." << std::endl;

    size_t const rSize = ZSTD_getFrameContentSize(str, sSize);
    auto rBuff = std::make_unique<char[]>(rSize);
    size_t const dSize = ZSTD_decompress(rBuff.get(), rSize, str, sSize);

    if (dSize != rSize)
        return "failed";

    // Success
    print_debug_message("Decompression success.");
    return std::string(rBuff.get(), rSize);
}

string decompress_string(string str) {
    return decompress_string(str.c_str(), str.size());
}

DYCORE_API const char* DyCore_decompress_string(const char* str,
                                                double _sSize) {
    static string returnBuffer;
    returnBuffer = decompress_string(str, _sSize);

    if (returnBuffer == "failed") {
        throw_error_event("Decompression failed.");
    }

    return returnBuffer.c_str();
}

size_t compress_bound(size_t size) {
    return ZSTD_compressBound(size);
}

int verify_project(string projectStr) {
    print_debug_message("Verifying project property...");
    try {
        json j = json::parse(projectStr);
        if (!j.contains("charts") || !j["charts"].contains("notes") ||
            !j.contains("version") || !j["version"].is_string()) {
            print_debug_message("Invalid project property: " + projectStr);
            return -1;
        }
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }
    return 0;
}

int verify_project_buffer(const char* buffer, size_t size) {
    if (!check_compressed(buffer, size)) {
        return verify_project(buffer);
    }
    return verify_project(decompress_string(buffer, size));
}

void __async_save_project(SaveProjectParams params) {
    namespace fs = std::filesystem;

    bool err = false;
    string errInfo = "";
    string projectString = "";
    try {
        projectString = get_project_string(params.projectProp);
        if (projectString == "" || verify_project(projectString) != 0) {
            print_debug_message("Invalid saving project property.");
            push_async_event(
                {PROJECT_SAVING, -1,
                 "Invalid project format. projectString: " + projectString +
                     "\nprojectProp: " + params.projectProp});
            return;
        }
    } catch (const std::exception& e) {
        print_debug_message("Encounter unknown errors. Details:" +
                            string(e.what()));
        push_async_event({PROJECT_SAVING, -1});
        return;
    }

    auto chartBuffer =
        std::make_unique<char[]>(compress_bound(projectString.size()));
    fs::path finalPath, tempPath;

    try {
        size_t cSize = get_project_buffer(projectString, chartBuffer.get(),
                                          params.compressionLevel);

        print_debug_message("filePath in hex:");
        for (size_t i = 0; i < params.filePath.size(); i++) {
            std::cout << std::hex << (int)params.filePath[i] << " ";
        }
        std::cout << std::endl;

        // Write to file using std::ofstream
        print_debug_message("Open file at:" + params.filePath);
        finalPath = fs::path(std::u8string((char8_t*)params.filePath.c_str()));
        tempPath = finalPath.parent_path() /
                   (finalPath.filename().string() + random_string(8) + ".tmp");
        std::ofstream file(tempPath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Error opening file: " +
                                     tempPath.string());
        } else {
            file.write(chartBuffer.get(), cSize);
            if (file.fail()) {
                throw std::runtime_error("Error writing to file: " +
                                         tempPath.string());
            }
            file.close();
        }

        print_debug_message("Verifying...");

        // Read the saved file.
        std::ifstream vfile(tempPath, std::ios::binary);
        if (!vfile.is_open()) {
            throw std::runtime_error(
                "Error opening saved file for verification: " +
                tempPath.string());
        } else {
            std::string buffer((std::istreambuf_iterator<char>(vfile)),
                               std::istreambuf_iterator<char>());
            vfile.close();

            if (verify_project_buffer(buffer.c_str(), buffer.size()) != 0) {
                throw std::exception("Saved file is corrupted.");
            }
        }

        fs::rename(tempPath, finalPath);
        print_debug_message("Project save completed.");

    } catch (const std::exception& e) {
        if (fs::exists(tempPath))
            fs::remove(tempPath);

        print_debug_message("Encounter errors. Details:" +
                            gb2312ToUtf8(e.what()));
        err = true;
        errInfo = gb2312ToUtf8(e.what());
    }

    push_async_event({PROJECT_SAVING, err ? -1 : 0, errInfo});
}

void save_project(const char* projectProp, const char* filePath,
                  double compressionLevel) {
    SaveProjectParams params;
    params.projectProp.assign(projectProp);
    params.filePath.assign(filePath);
    params.compressionLevel = (int)compressionLevel;
    std::thread t([=]() { __async_save_project(params); });
    t.detach();
    return;
}

DYCORE_API double DyCore_save_project(const char* projectProp,
                                      const char* filePath,
                                      double compressionLevel) {
    namespace fs = std::filesystem;

    if (!filePath || strlen(filePath) == 0) {
        throw_error_event("File path is empty.");
        return -1;
    }

    fs::path path = fs::path((char8_t*)filePath);
    fs::path parentDir = path.parent_path();
    if (!parentDir.empty() && !fs::exists(parentDir)) {
        throw_error_event("Parent directory does not exist: " +
                          parentDir.string());
        return -1;
    }

    save_project(projectProp, filePath, compressionLevel);
    return 0;
}

string get_project_string(string projectProp) {
    json js;
    try {
        js = json::parse(projectProp);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        print_debug_message("Project property: " + projectProp);
        return "";
    }

    // Get the final notes array.
    mtxSaveProject.lock();
    std::vector<Note> notes;
    for (auto note : currentNoteMap)
        if (note.second.noteType != 3)
            notes.push_back(note.second);
    mtxSaveProject.unlock();

    js["charts"]["notes"] = notes;
    return nlohmann::to_string(js);
}

double get_project_buffer(string projectString, char* targetBuffer,
                          double compressionLevel) {
    return DyCore_compress_string(projectString.c_str(), targetBuffer,
                                  compressionLevel);
}

// Insert the notes array into the project property.
// Might be called asynchronously.
DYCORE_API double DyCore_get_project_buffer(const char* projectProp,
                                            char* targetBuffer,
                                            double compressionLevel) {
    string project = get_project_string(projectProp);

    return get_project_buffer(project, targetBuffer, compressionLevel);
}
