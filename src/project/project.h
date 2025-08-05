#pragma once
#include <zstd.h>

#include <array>
#include <json.hpp>
#include <string>

#include "note.h"
#include "timing.h"

struct SaveProjectParams {
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
void to_json(nlohmann::json &j, const ChartMetadata &meta);
void from_json(const nlohmann::json &j, ChartMetadata &meta);

struct ChartPath {
    std::string music;
    std::string video;
    std::string image;
};
void to_json(nlohmann::json &j, const ChartPath &path);
void from_json(const nlohmann::json &j, ChartPath &path);

struct Chart {
    ChartMetadata metadata;
    ChartPath path;
    std::vector<Note> notes;
    std::vector<TimingPoint> timingPoints;
};
void to_json(nlohmann::json &j, const Chart &chart);
void from_json(const nlohmann::json &j, Chart &chart);

struct Project {
    std::string version;
    nlohmann::json metadata;
    std::vector<Chart> charts;
};
void to_json(nlohmann::json &j, const Project &project);
void from_json(const nlohmann::json &j, Project &project);

void __async_save_project(SaveProjectParams params);

void load_project(const char *filePath);
void save_project(const char *filePath, double compressionLevel);

string get_project_string();

double get_project_buffer(const std::string &projectString, char *targetBuffer,
                          double compressionLevel);

void chart_set_metadata(const ChartMetadata &metaData);
ChartMetadata chart_get_metadata();
void chart_set_path(const ChartPath &path);
ChartPath chart_get_path();
void project_set_metadata(const nlohmann::json &metaData);
nlohmann::json project_get_metadata();
string project_get_version();
void project_set_version(const string &ver);

void project_set_current_chart(const int &index);
