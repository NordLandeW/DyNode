#pragma once
#include <windows.h>
#include <zstd.h>

#include <codecvt>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <mutex>
#include <sol/sol.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "delaunator.hpp"
#include "json.hpp"
#include "pugixml.hpp"


#if !defined(_MSC_VER)
#define EXPORTED_FN __attribute__((visibility("default")))
#else
#define EXPORTED_FN __declspec(dllexport)
#endif

#define DYCORE_API extern "C" EXPORTED_FN

using json = nlohmann::json;
using std::map;
using std::string;
using std::vector;

/// Notes Management

namespace dyn {

struct Note {
    double time;
    int side;
    double width;
    double position;
    double lastTime;
    int noteType;
    string inst;
    double beginTime;
};

inline void from_json(const json &j, Note &n) {
    j.at("time").get_to(n.time);
    j.at("side").get_to(n.side);
    j.at("width").get_to(n.width);
    j.at("position").get_to(n.position);
    j.at("lastTime").get_to(n.lastTime);
    j.at("noteType").get_to(n.noteType);
    j.at("inst").get_to(n.inst);
    j.at("beginTime").get_to(n.beginTime);
}

inline void to_json(json &j, const Note &n) {
    j = json{{"time", n.time},         {"side", n.side},
             {"width", n.width},       {"position", n.position},
             {"lastTime", n.lastTime}, {"noteType", n.noteType}};
}
}  // namespace dyn

/// Async Event Management

enum ASYNC_EVENT_TYPE { PROJECT_SAVING };

struct AsyncEvent {
    ASYNC_EVENT_TYPE type;
    int status;
};
inline void to_json(json &j, const AsyncEvent &a) {
    j = json{{"type", a.type}, {"status", a.status}};
}

extern std::vector<AsyncEvent> asyncEventStack;
extern std::mutex mtxSaveProject;

DYCORE_API double DyCore_has_async_event();

DYCORE_API const char *DyCore_get_async_event();

/// Project Management

namespace dyn {

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
}  // namespace dyn

/// ZSTD Stuff

/*! CHECK
 * Check that the condition holds. If it doesn't print a message and die.
 */
#define CHECK(cond, ...)                                                    \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d CHECK(%s) failed: ", __FILE__, __LINE__, \
                    #cond);                                                 \
            fprintf(stderr, "" __VA_ARGS__);                                \
            fprintf(stderr, "\n");                                          \
            throw;                                                          \
        }                                                                   \
    } while (0)

/*! CHECK_ZSTD
 * Check the zstd error code and die if an error occurred after printing a
 * message.
 */
#define CHECK_ZSTD(fn)                                           \
    do {                                                         \
        size_t const err = (fn);                                 \
        CHECK(!ZSTD_isError(err), "%s", ZSTD_getErrorName(err)); \
    } while (0)

/// Main Functions

DYCORE_API const char *DyCore_init();

DYCORE_API const char *DyCore_delaunator(char *in_struct);

DYCORE_API double DyCore_get_project_buffer(const char *projectProp,
                                            char *targetBuffer,
                                            double compressionLevel);