#pragma once

#include "project.h"

inline constexpr int DYN_FILE_FORMAT_VERSION = 1;

int project_import_dyn(const char* filePath, Project& project);

int chart_import_dyn(const char* filePath, bool importInfo, bool importTiming);
