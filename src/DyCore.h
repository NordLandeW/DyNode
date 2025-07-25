#pragma once
#include <windows.h>
#include <zstd.h>

#include <map>
#include <sol/sol.hpp>
#include <string>
#include <vector>

#include "api.h"
#include "json.hpp"
#include "pugixml.hpp"

using json = nlohmann::json;
using std::map;
using std::string;
using std::vector;

DYCORE_API const char *DyCore_init();