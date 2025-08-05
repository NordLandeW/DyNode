
#include "project.h"

#include <zstd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>

#include "compress.h"
#include "format/dyn.h"
#include "gm.h"
#include "note.h"
#include "projectManager.h"
#include "utils.h"

ChartMetadata chartMetaData;

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

    bool err = false;
    string errInfo = "";
    string projectString = "";
    try {
        // Update current chart.
        get_project_manager().update_current_chart();
        projectString = get_project_manager().dump();
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

    try {
        size_t cSize = get_project_buffer(projectString, chartBuffer.get(),
                                          params.compressionLevel);

        // Write to file using std::ofstream
        print_debug_message("Open file at:" + params.filePath);
        finalPath = convert_char_to_path(params.filePath.c_str());
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

    } catch (const std::exception &e) {
        if (fs::exists(tempPath))
            fs::remove(tempPath);

        print_debug_message("Encounter errors. Details:" +
                            gb2312ToUtf8(e.what()));
        err = true;
        errInfo = gb2312ToUtf8(e.what());
    }

    push_async_event({PROJECT_SAVING, err ? -1 : 0, errInfo});
}

void load_project(const char *filePath) {
    get_project_manager().load_project_from_file(filePath);
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
    get_project_manager().set_chart_metadata(metaData);
}
ChartMetadata chart_get_metadata() {
    return get_project_manager().get_chart_metadata();
}
void chart_set_path(const ChartPath &path) {
    get_project_manager().set_chart_path(path);
}
ChartPath chart_get_path() {
    return get_project_manager().get_chart_path();
}
void project_set_metadata(const nlohmann::json &metaData) {
    get_project_manager().set_project_metadata(metaData);
}
nlohmann::json project_get_metadata() {
    return get_project_manager().get_project_metadata();
}
void project_set_current_chart(const int &index) {
    get_project_manager().set_current_chart(index);
}
string project_get_version() {
    return get_project_manager().get_version();
}
void project_set_version(const string &ver) {
    get_project_manager().set_version(ver);
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
        chart.timingPoints.push_back({tp["offset"].get<double>(),
                                      60000.0 / tp["bpm"].get<double>(),
                                      tp["meter"].get<int>()});
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
