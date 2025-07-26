#pragma once
#include <zstd.h>

#include <string>

#include "api.h"

using std::string;


struct SaveProjectParams {
    std::string projectProp;
    std::string filePath;
    int compressionLevel;
};

void __async_save_project(SaveProjectParams params);

void save_project(const char *projectProp, const char *filePath,
                  double compressionLevel);

DYCORE_API double DyCore_save_project(const char *projectProp,
                                      const char *filePath,
                                      double compressionLevel);

string get_project_string(string projectProp);

double get_project_buffer(string projectString, char *targetBuffer,
                          double compressionLevel);