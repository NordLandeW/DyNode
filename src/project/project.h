#pragma once
#include <zstd.h>

#include <array>
#include <json.hpp>
#include <string>

#include "api.h"
#include "format/dyn.h"
#include "note.h"
#include "timing.h"

struct SaveProjectParams {
    std::string projectProp;
    std::string filePath;
    int compressionLevel;
};

struct Project;
struct Chart;
struct ChartMetadata;
struct ChartPath;

struct ChartMetadata {
    std::string title, artist, charter;
    std::array<std::string, 2> sideType;
    int difficulty;
};
inline void to_json(nlohmann::json &j, const ChartMetadata &meta) {
    j["title"] = meta.title;
    j["sideType"] = meta.sideType;
    j["difficulty"] = meta.difficulty;
    j["artist"] = meta.artist;
    j["charter"] = meta.charter;
}
inline void from_json(const nlohmann::json &j, ChartMetadata &meta) {
    j.at("title").get_to(meta.title);
    j.at("artist").get_to(meta.artist);
    j.at("charter").get_to(meta.charter);
    j.at("sideType").get_to(meta.sideType);
    j.at("difficulty").get_to(meta.difficulty);
}

struct ChartPath {
    std::string music;
    std::string video;
    std::string image;
};
inline void to_json(nlohmann::json &j, const ChartPath &path) {
    j["music"] = path.music;
    j["video"] = path.video;
    j["image"] = path.image;
}
inline void from_json(const nlohmann::json &j, ChartPath &path) {
    j.at("music").get_to(path.music);
    j.at("video").get_to(path.video);
    j.at("image").get_to(path.image);
}

struct Chart {
    ChartMetadata metadata;
    ChartPath path;
    std::vector<Note> notes;
    std::vector<TimingPoint> timingPoints;
};
inline void to_json(nlohmann::json &j, const Chart &chart) {
    j["metadata"] = chart.metadata;
    j["path"] = chart.path;
    j["notes"] = nlohmann::json::array();
    for (const auto &note : chart.notes) {
        j["notes"].push_back(NoteExportView(note));
    }
    j["timingPoints"] = chart.timingPoints;
}
inline void from_json(const nlohmann::json &j, Chart &chart) {
    j.at("metadata").get_to(chart.metadata);
    j.at("path").get_to(chart.path);
    j.at("notes").get_to(chart.notes);
    j.at("timingPoints").get_to(chart.timingPoints);
}

struct Project {
    std::string version;
    nlohmann::json metadata;
    std::vector<Chart> charts;
};
inline void to_json(nlohmann::json &j, const Project &project) {
    j["version"] = project.version;
    j["formatVersion"] = DYN_FILE_FORMAT_VERSION;
    j["metadata"] = project.metadata;
    j["charts"] = project.charts;
}
inline void from_json(const nlohmann::json &j, Project &project) {
    j.at("version").get_to(project.version);
    j.at("metadata").get_to(project.metadata);
    j.at("charts").get_to(project.charts);
}

void __async_save_project(SaveProjectParams params);

void save_project(const char *projectProp, const char *filePath,
                  double compressionLevel);

DYCORE_API double DyCore_save_project(const char *projectProp,
                                      const char *filePath,
                                      double compressionLevel);

std::string get_project_string(const std::string &projectProp);
std::string get_notes_array_string();

double get_project_buffer(const std::string &projectString, char *targetBuffer,
                          double compressionLevel);

void project_set_metadata(const ChartMetadata &metaData);
ChartMetadata project_get_metadata();