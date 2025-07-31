#define NOMINMAX
#include "utils.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <taskflow/taskflow.hpp>

#include "api.h"

// Prints a debug message to the console, prefixed with "[DyCore] ".
void print_debug_message(std::string str) {
    std::cout << "[DyCore] " << str << std::endl;
}

// Converts a wide string (wstring) to a UTF-8 encoded string.
std::string wstringToUtf8(const std::wstring& wstr) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0,
                                  nullptr, nullptr);
    if (len == 0) {
        return "";
    }
    std::string str(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, nullptr,
                        nullptr);
    str.resize(len - 1);  // 移除末尾的 '\0'
    return str;
}

// Converts a UTF-8 encoded string to a wide string (wstring).
std::wstring s2ws(const std::string& str) {
    int size_needed =
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0],
                        size_needed);
    return wstrTo;
}

// Converts a GB2312 encoded string to a UTF-8 encoded string.
std::string gb2312ToUtf8(const std::string& gbStr) {
    if (gbStr.empty())
        return "";

    int wlen = MultiByteToWideChar(936, 0, gbStr.c_str(), (int)gbStr.size(),
                                   nullptr, 0);
    if (wlen == 0)
        return "";

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(936, 0, gbStr.c_str(), (int)gbStr.size(), &wstr[0],
                        wlen);

    int utf8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, nullptr,
                                      0, nullptr, nullptr);
    if (utf8len == 0)
        return "";

    std::string utf8str(utf8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &utf8str[0], utf8len,
                        nullptr, nullptr);

    return utf8str;
}

// Retrieves the last modification time of a file as a formatted string.
// The format is "YYYY_MM_DD_HH_MM_SS".
std::string get_file_modification_time(char* file_path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    auto ftime = fs::last_write_time(fs::path((char8_t*)file_path), ec);

    if (ec) {
        std::cout << "Error reading file time: " << file_path << std::endl;
        return "failed";
    }

    auto ftime_sec = std::chrono::floor<std::chrono::seconds>(ftime);

    return std::format("{:%Y_%m_%d_%H_%M_%S}", ftime_sec);
}

// Generates a random string of a specified length.
// The string consists of alphanumeric characters.
std::string random_string(int length) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    static std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    std::string str(length, 0);
    for (int i = 0; i < length; ++i) {
        str[i] = charset[dis(gen)];
    }
    return str;
}

// Copies a block of memory from a source address to a destination address.
DYCORE_API double DyCore_buffer_copy(void* dst, void* src, double size) {
    memcpy(dst, src, (size_t)size);
    return 0;
}

// Sorts an array of std::pair<double, double> in ascending order based on the
// first element of the pair.
DYCORE_API double DyCore_index_sort(void* data, double size) {
    auto* pair_data = static_cast<std::pair<double, double>*>(data);
    std::sort(pair_data, pair_data + (size_t)size);
    return 0;
}

// Sorts an array of doubles.
// The sorting order is determined by the 'type' parameter.
//
// @param data A pointer to the array of doubles.
// @param size The number of elements in the array.
// @param type If true, sorts in ascending order; otherwise, sorts in descending
// order.
DYCORE_API double DyCore_quick_sort(void* data, double size, double type) {
    double* arr = static_cast<double*>(data);
    if (type) {
        std::sort(arr, arr + (size_t)size);
    } else {
        std::sort(arr, arr + (size_t)size, std::greater<double>());
    }
    return 0;
}

DYCORE_API const char* DyCore_random_string(double length) {
    static std::string str;
    str = random_string(int(length));
    return str.c_str();
}