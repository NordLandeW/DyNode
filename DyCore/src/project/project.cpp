
#include "project.h"

#include <zstd.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <system_error>

#include "compress.h"
#include "format/dyn.h"
#include "gm.h"
#include "note.h"
#include "projectManager.h"
#include "timer.h"
#include "timing.h"
#include "utils.h"

namespace {

constexpr int MAX_EMERGENCY_PROJECT_BACKUPS_COUNT = 3;

std::mutex projectSaveMutex;

#ifdef _WIN32
void throw_last_windows_error(const char *message) {
    throw std::system_error(static_cast<int>(GetLastError()),
                            std::system_category(), message);
}
#endif

void write_file_durably(const std::filesystem::path &path, const char *data,
                        size_t size) {
#ifdef _WIN32
    HANDLE handle = CreateFileW(
        path.wstring().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throw_last_windows_error("Error opening file for writing.");
    }

    size_t writtenTotal = 0;
    while (writtenTotal < size) {
        const size_t remaining = size - writtenTotal;
        const DWORD chunkSize =
            static_cast<DWORD>(std::min<size_t>(remaining, MAXDWORD));
        DWORD written = 0;
        if (!WriteFile(handle, data + writtenTotal, chunkSize, &written,
                       nullptr)) {
            const DWORD error = GetLastError();
            CloseHandle(handle);
            throw std::system_error(static_cast<int>(error),
                                    std::system_category(),
                                    "Error writing to file.");
        }
        if (written == 0) {
            CloseHandle(handle);
            throw std::runtime_error("Error writing to file.");
        }
        writtenTotal += written;
    }

    if (!FlushFileBuffers(handle)) {
        const DWORD error = GetLastError();
        CloseHandle(handle);
        throw std::system_error(static_cast<int>(error),
                                std::system_category(),
                                "Error flushing file to disk.");
    }

    if (!CloseHandle(handle)) {
        throw_last_windows_error("Error closing file.");
    }
#else
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Error opening file for writing.");
    }
    file.write(data, size);
    file.flush();
    if (file.fail()) {
        throw std::runtime_error("Error writing to file.");
    }
    file.close();
    if (file.fail()) {
        throw std::runtime_error("Error closing file.");
    }
#endif
}

void replace_file_durably(const std::filesystem::path &tempPath,
                          const std::filesystem::path &finalPath) {
#ifdef _WIN32
    if (!MoveFileExW(tempPath.wstring().c_str(), finalPath.wstring().c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw_last_windows_error("Error replacing project file.");
    }
#else
    std::filesystem::rename(tempPath, finalPath);
#endif
}

void delete_file_durably(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
        return;
    }
#ifdef _WIN32
    if (!DeleteFileW(path.wstring().c_str())) {
        throw_last_windows_error("Error deleting file.");
    }
#else
    if (!std::filesystem::remove(path)) {
        throw std::runtime_error("Error deleting file.");
    }
#endif
}

std::filesystem::path emergency_backup_path(
    const std::filesystem::path &finalPath, int version) {
    std::filesystem::path backupName = finalPath.stem();
    backupName += ".bak.";
    backupName += std::to_string(version);
    backupName += finalPath.extension();
    return finalPath.parent_path() / "backups" / backupName;
}

}  // namespace

void backup_existing_project_file(const std::filesystem::path &finalPath) {
    namespace fs = std::filesystem;

    if (!fs::exists(finalPath)) {
        return;
    }

    const fs::path backupDir = finalPath.parent_path() / "backups";
    fs::create_directories(backupDir);

    static_assert(MAX_EMERGENCY_PROJECT_BACKUPS_COUNT > 0);

    delete_file_durably(
        emergency_backup_path(finalPath, MAX_EMERGENCY_PROJECT_BACKUPS_COUNT));
    delete_file_durably(emergency_backup_path(
        finalPath, MAX_EMERGENCY_PROJECT_BACKUPS_COUNT - 1));

    for (int version = MAX_EMERGENCY_PROJECT_BACKUPS_COUNT - 2; version >= 0;
         --version) {
        const fs::path currentPath = emergency_backup_path(finalPath, version);
        if (!fs::exists(currentPath)) {
            continue;
        }
        replace_file_durably(currentPath,
                             emergency_backup_path(finalPath, version + 1));
    }

    replace_file_durably(finalPath, emergency_backup_path(finalPath, 0));
}

// Verifies the integrity of a project string by checking its JSON structure.
//
// @param projectStr The project data as a JSON string.
// @return 0 if the project is valid, -1 otherwise.
int verify_project(string projectStr) {
    print_debug_message("Verifying project property...");
    try {
        json j = json::parse(projectStr);
        if (!j.contains("version") || !j["version"].is_string()) {
            print_debug_message("Invalid project property: " + projectStr);
            return -1;
        }
    } catch (json::exception &e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }
    return 0;
}

// Verifies the integrity of a project buffer.
// It handles both compressed and uncompressed data.
//
// @param buffer A pointer to the project data buffer.
// @param size The size of the buffer.
// @return 0 if the project is valid, -1 otherwise.
int verify_project_buffer(const char *buffer, size_t size) {
    if (!check_compressed(buffer, size)) {
        return verify_project(buffer);
    }
    return verify_project(decompress_string(buffer, size));
}

// Asynchronously saves the project to a file.
// This function is intended to be run in a separate thread.
void __async_save_project(SaveProjectParams params) {
    namespace fs = std::filesystem;

    std::lock_guard<std::mutex> saveLock(projectSaveMutex);

    bool err = false;
    string errInfo = "";
    string projectString = "";
    try {
        // Update current chart.
        ProjectManager::inst().update_current_chart();
        projectString = ProjectManager::inst().dump();
        if (projectString == "" || verify_project(projectString) != 0) {
            print_debug_message("Invalid saving project property.");
            push_async_event(
                {PROJECT_SAVING, -1,
                 "Invalid project format. projectString: " + projectString});
            return;
        }
    } catch (const std::exception &e) {
        print_debug_message("Encounter unknown errors. Details:" +
                            string(e.what()));
        push_async_event({PROJECT_SAVING, -1});
        return;
    }

    auto chartBuffer =
        std::make_unique<char[]>(compress_bound(projectString.size()));
    fs::path finalPath, tempPath;
    bool tempFileVerified = false;

    try {
        const double compressedSize = get_project_buffer(
            projectString, chartBuffer.get(), params.compressionLevel);
        if (compressedSize < 0) {
            throw std::runtime_error("Error compressing project data.");
        }
        size_t cSize = static_cast<size_t>(compressedSize);

        print_debug_message("Open file at:" + params.filePath);
        finalPath = convert_char_to_path(params.filePath.c_str());
        fs::path tempName = finalPath.filename();
        tempName += random_string(8);
        tempName += ".tmp";

        tempPath = finalPath.parent_path() / tempName;
        write_file_durably(tempPath, chartBuffer.get(), cSize);

        print_debug_message("Verifying...");

        // Read the saved file.
        std::ifstream vfile(tempPath, std::ios::binary);
        if (!vfile.is_open()) {
            print_debug_message(L"Error opening saved file for verification: " +
                                tempPath.wstring());
            throw std::runtime_error(
                "Error opening saved file for verification.");
        } else {
            std::string buffer((std::istreambuf_iterator<char>(vfile)),
                               std::istreambuf_iterator<char>());
            vfile.close();

            if (verify_project_buffer(buffer.c_str(), buffer.size()) != 0) {
                throw std::runtime_error("Saved file is corrupted.");
            }
        }
        tempFileVerified = true;

        backup_existing_project_file(finalPath);
        replace_file_durably(tempPath, finalPath);
        print_debug_message("Project save completed.");
    } catch (const std::exception &e) {
        if (!tempFileVerified && fs::exists(tempPath))
            fs::remove(tempPath);

        print_debug_message("Encounter errors. Details:" +
                            gb2312ToUtf8(e.what()));
        err = true;
        errInfo = gb2312ToUtf8(e.what());
    }

    push_async_event({PROJECT_SAVING, err ? -1 : 0, errInfo});
}

void load_project(const char *filePath) {
    TIMER_SCOPE("load_project");

    ProjectManager::inst().load_project_from_file(filePath);
}

// Initiates an asynchronous save of the project.
void save_project(const char *filePath, double compressionLevel) {
    SaveProjectParams params;
    params.filePath.assign(filePath);
    params.compressionLevel = (int)compressionLevel;
    std::thread t([=]() { __async_save_project(params); });
    t.detach();
    return;
}

// Compresses the project string into a buffer.
//
// @param projectString The project data as a string.
// @param targetBuffer The buffer to store the compressed data.
// @param compressionLevel The compression level.
// @return The size of the compressed data.
double get_project_buffer(const string &projectString, char *targetBuffer,
                          double compressionLevel) {
    return DyCore_compress_string(projectString.c_str(), targetBuffer,
                                  compressionLevel);
}

// =============================================================================
// Project Manager Wrapper Functions
// =============================================================================

void chart_set_metadata(const ChartMetadata &metaData) {
    ProjectManager::inst().set_chart_metadata(metaData);
}
ChartMetadata chart_get_metadata() {
    return ProjectManager::inst().get_chart_metadata();
}
void chart_set_path(const ChartPath &path) {
    ProjectManager::inst().set_chart_path(path);
}
ChartPath chart_get_path() {
    return ProjectManager::inst().get_chart_path();
}
int chart_load_audio(const char *filePath) {
    return ProjectManager::inst().load_chart_audio(filePath);
}
void chart_unload_audio() {
    ProjectManager::inst().unload_chart_audio();
}
void project_set_metadata(const nlohmann::json &metaData) {
    ProjectManager::inst().set_project_metadata(metaData);
}
nlohmann::json project_get_metadata() {
    return ProjectManager::inst().get_project_metadata();
}
void project_set_current_chart(const int &index) {
    ProjectManager::inst().set_current_chart(index);
}
string project_get_version() {
    return ProjectManager::inst().get_version();
}
void project_set_version(const string &ver) {
    ProjectManager::inst().set_version(ver);
}

string project_get_full_path(const char *relativePath) {
    auto &projectManager = ProjectManager::inst();
    return projectManager.get_full_path(relativePath);
}

// =============================================================================
// JSON Serialization/Deserialization Functions
// =============================================================================

void to_json(nlohmann::json &j, const ChartMetadata &meta) {
    j["title"] = meta.title;
    j["sideType"] = meta.sideType;
    j["difficulty"] = meta.difficulty;
    j["artist"] = meta.artist;
    j["charter"] = meta.charter;
}
void from_json(const nlohmann::json &j, ChartMetadata &meta) {
    j.at("title").get_to(meta.title);
    j.at("artist").get_to(meta.artist);
    j.at("charter").get_to(meta.charter);
    j.at("sideType").get_to(meta.sideType);
    j.at("difficulty").get_to(meta.difficulty);
}

void to_json(nlohmann::json &j, const ChartPath &path) {
    j["music"] = path.music;
    j["video"] = path.video;
    j["image"] = path.image;
}
void from_json(const nlohmann::json &j, ChartPath &path) {
    j.at("music").get_to(path.music);
    j.at("video").get_to(path.video);
    j.at("image").get_to(path.image);
}

void to_json(nlohmann::json &j, const Chart &chart) {
    j["metadata"] = chart.metadata;
    j["path"] = chart.path;
    j["notes"] = nlohmann::json::array();
    for (const auto &note : chart.notes) {
        j["notes"].push_back(NoteExportView(note));
    }
    j["timingPoints"] = nlohmann::json::array();
    for (const auto &tp : chart.timingPoints) {
        j["timingPoints"].push_back(TimingPointExportView(tp));
    }
}
void from_json(const nlohmann::json &j, Chart &chart) {
    j.at("metadata").get_to(chart.metadata);
    j.at("path").get_to(chart.path);
    chart.notes.clear();
    for (const auto &note : j["notes"]) {
        Note n;
        note["side"].get_to<int>(n.side);
        note["type"].get_to<int>(n.type);
        note["time"].get_to<double>(n.time);
        note["length"].get_to<double>(n.lastTime);
        note["width"].get_to<double>(n.width);
        note["position"].get_to<double>(n.position);
        chart.notes.push_back(n);
    }
    chart.timingPoints.clear();
    for (const auto &tp : j["timingPoints"]) {
        TimingPoint tpObj;
        TimingPointImportView view(tpObj);
        from_json(tp, view);
        chart.timingPoints.push_back(tpObj);
    }
}

void to_json(nlohmann::json &j, const Project &project) {
    j["version"] = project.version;
    j["metadata"] = project.metadata;
    j["charts"] = project.charts;
    j["formatVersion"] = DYN_FILE_FORMAT_VERSION;
}
void from_json(const nlohmann::json &j, Project &project) {
    j.at("version").get_to(project.version);
    j.at("metadata").get_to(project.metadata);
    j.at("charts").get_to(project.charts);
}
