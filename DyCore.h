#pragma once
#include <vector>
#include <string>
#include <map>
#include "json.hpp"
#include "pugixml.hpp"
#include "delaunator.hpp"
#include <iostream>

#if !defined( _MSC_VER)
#define EXPORTED_FN __attribute__((visibility("default")))
#else
#define EXPORTED_FN __declspec(dllexport)
#endif

#define DYCORE_API extern "C" EXPORTED_FN

using json = nlohmann::json;
using std::string;
using std::vector;
using std::map;