

#include "compress.h"

#include <zstd.h>

#include <algorithm>
#include <iostream>
#include <string>

#include "api.h"
#include "gm.h"
#include "utils.h"

using std::string;

// Compresses a string using Zstandard (zstd) algorithm.
//
// @param str The input string to compress.
// @param targetBuffer The buffer to store the compressed data.
// @param compressionLevel The compression level (0-22). Level 0 skips
// compression.
// @return The size of the compressed data in bytes, or -1 on error.
DYCORE_API double DyCore_compress_string(const char* str, char* targetBuffer,
                                         double compressionLevel) {
    if (!str || !targetBuffer) {
        print_debug_message("Error: Null pointer passed to compress_string.");
        return -1;
    }
    compressionLevel = std::clamp((int)compressionLevel, 0, 22);
    // Skip the compression if the compression level is 0.
    if ((int)compressionLevel == 0) {
        size_t fSize = strlen(str);
        memcpy(targetBuffer, str, fSize);
        std::cout << "[DyCore] Compression level set to 0. Skip compression."
                  << std::endl;
        return fSize;
    }

    size_t fSize = strlen(str);
    size_t cBuffSize = ZSTD_compressBound(fSize);

    auto fBuff = std::make_unique<char[]>(fSize);
    auto cBuff = std::make_unique<char[]>(cBuffSize);

    memcpy(fBuff.get(), str, fSize);

    std::cout << "[DyCore] Start compressing..." << std::endl;

    size_t const cSize = ZSTD_compress(cBuff.get(), cBuffSize, fBuff.get(),
                                       fSize, (int)compressionLevel);

    std::cout << "[DyCore] Finish compressing, checking..." << std::endl;

    try {
        CHECK_ZSTD(cSize);
    } catch (...) {
        return -1;
    };

    std::cout << "[DyCore] No error found. Success. " << fSize << "->" << cSize
              << "(" << ((double)cSize / fSize * 100.0) << "%)" << std::endl;

    // Success
    memcpy(targetBuffer, cBuff.get(), cSize);

    return (double)cSize;
}

// Checks if a given data buffer appears to be compressed with zstd.
//
// @param str A pointer to the data buffer.
// @param _sSize The size of the data buffer.
// @return True if the data is likely zstd-compressed, false otherwise.
bool check_compressed(const char* str, double _sSize) {
    size_t sSize = (size_t)_sSize;
    unsigned long long const rSize = ZSTD_getFrameContentSize(str, sSize);
    return rSize != ZSTD_CONTENTSIZE_ERROR && rSize != ZSTD_CONTENTSIZE_UNKNOWN;
}

// Checks if a data buffer is compressed using zstd.
//
// @param str A pointer to the data buffer.
// @param _sSize The size of the data buffer.
// @return 0.0 if compressed, -1.0 otherwise.
DYCORE_API double DyCore_is_compressed(const char* str, double _sSize) {
    size_t sSize = (size_t)_sSize;
    return check_compressed(str, sSize) ? 0.0 : -1.0;
}

// Decompresses a zstd-compressed string.
//
// @param str A pointer to the compressed data.
// @param _sSize The size of the compressed data.
// @return The decompressed string, or "failed" on error.
string decompress_string(const char* str, double _sSize) {
    size_t sSize = (size_t)_sSize;
    if (!check_compressed(str, sSize)) {
        std::cout << "[DyCore] Not a zstd file!" << std::endl;

        return "failed";
    }

    std::cout << "[DyCore] Start decompressing..." << std::endl;

    size_t const rSize = ZSTD_getFrameContentSize(str, sSize);
    auto rBuff = std::make_unique<char[]>(rSize);
    size_t const dSize = ZSTD_decompress(rBuff.get(), rSize, str, sSize);

    if (dSize != rSize)
        return "failed";

    // Success
    print_debug_message("Decompression success.");
    return std::string(rBuff.get(), rSize);
}

// Decompresses a zstd-compressed string.
//
// @param str The compressed string.
// @return The decompressed string, or "failed" on error.
string decompress_string(string str) {
    return decompress_string(str.c_str(), str.size());
}

// Decompresses a zstd-compressed string and returns it as a C-style string.
//
// @param str A pointer to the compressed data.
// @param _sSize The size of the compressed data.
// @return A pointer to the decompressed string, or an error message.
DYCORE_API const char* DyCore_decompress_string(const char* str,
                                                double _sSize) {
    static string returnBuffer;
    returnBuffer = decompress_string(str, _sSize);

    if (returnBuffer == "failed") {
        throw_error_event("Decompression failed.");
    }

    return returnBuffer.c_str();
}

// Returns the maximum compressed size in the worst-case scenario for a given
// input size.
size_t compress_bound(size_t size) {
    return ZSTD_compressBound(size);
}
