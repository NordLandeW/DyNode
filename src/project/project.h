#pragma once
#include <zstd.h>

#include <array>
#include <json.hpp>
#include <string>

#include "api.h"

using std::string;

struct SaveProjectParams {
    std::string projectProp;
    std::string filePath;
    int compressionLevel;
};

struct ChartMetaData {
    std::string title;
    std::array<std::string, 2> sideType;
    int difficulty;

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["title"] = title;
        j["sidetype"] = sideType;
        j["difficulty"] = difficulty;
        return j;
    }

    string dump() const {
        return to_json().dump();
    }
};

void __async_save_project(SaveProjectParams params);

void save_project(const char *projectProp, const char *filePath,
                  double compressionLevel);

DYCORE_API double DyCore_save_project(const char *projectProp,
                                      const char *filePath,
                                      double compressionLevel);

string get_project_string(string projectProp);
string get_notes_array_string();

double get_project_buffer(string projectString, char *targetBuffer,
                          double compressionLevel);

void project_set_metadata(const ChartMetaData &metaData);
ChartMetaData project_get_metadata();