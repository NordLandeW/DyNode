#pragma once
#include <zstd.h>

#include <string>

#include "api.h"

using std::string;

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

DYCORE_API double DyCore_get_project_buffer(const char *projectProp,
                                            char *targetBuffer,
                                            double compressionLevel);

DYCORE_API double DyCore_compress_string(const char *src, char *dst,
                                         double compressionLevel);

DYCORE_API double DyCore_is_compressed(const char *str, double _sSize);

string decompress_string(const char *str, double _sSize);
string decompress_string(string str);

DYCORE_API const char *DyCore_decompress_string(const char *str, double _sSize);