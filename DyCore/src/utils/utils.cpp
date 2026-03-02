
#include <string>
#define NOMINMAX
#include <Windows.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <random>
#include <taskflow/taskflow.hpp>

#include "analytics.h"
#include "utils.h"

namespace fs = std::filesystem;

std::mutex g_debug_print_mutex;

// Prints a debug message to the console, prefixed with "[DyCore] ".
void print_debug_message(std::string str) {
    // iostream output is not guaranteed to be atomic across threads; serialize
    // writes so log lines won't interleave.
    const std::scoped_lock lock(g_debug_print_mutex);
    std::cout << "[DyCore] " << str << std::endl;
}
void print_debug_message(std::wstring str) {
    print_debug_message(wstringToUtf8(str));
}

// Converts a char* file path to a std::filesystem::path.
// This function assumes the input is a UTF-8 encoded string.
// Mainly used by Gamemaker API.
fs::path convert_char_to_path(const char* filePath) {
    // Convert char* to std::u8string, then to fs::path
    return fs::path(std::u8string((char8_t*)filePath));
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
    str.resize(len - 1);
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
    constexpr const char* kFallbackTimestamp = "1970_01_01_00_00_00";

    namespace fs = std::filesystem;
    try {
        std::error_code ec;
        auto ftime = fs::last_write_time(fs::path((char8_t*)file_path), ec);

        if (ec) {
            print_debug_message("Error reading file time: " +
                                std::string(file_path));
            report_exception_error(
                "std::runtime_error",
                std::runtime_error("std::filesystem::last_write_time failed."));
            return kFallbackTimestamp;
        }

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
        const auto sys_time =
            std::chrono::clock_cast<std::chrono::system_clock>(ftime);
#else
        const auto sys_time =
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() +
                std::chrono::system_clock::now());
#endif
        const auto sys_time_sec =
            std::chrono::floor<std::chrono::seconds>(sys_time);
        const std::time_t time_value =
            std::chrono::system_clock::to_time_t(sys_time_sec);

        std::tm local_time{};
#if defined(_WIN32)
        if (localtime_s(&local_time, &time_value) != 0) {
            print_debug_message("Failed to convert file time: " +
                                std::string(file_path));
            report_exception_error("std::runtime_error",
                                   std::runtime_error("localtime_s failed."));
            return kFallbackTimestamp;
        }
#else
        if (localtime_r(&time_value, &local_time) == nullptr) {
            print_debug_message("Failed to convert file time: " +
                                std::string(file_path));
            report_exception_error("std::runtime_error",
                                   std::runtime_error("localtime_r failed."));
            return kFallbackTimestamp;
        }
#endif

        return std::format("{:04}_{:02}_{:02}_{:02}_{:02}_{:02}",
                           local_time.tm_year + 1900, local_time.tm_mon + 1,
                           local_time.tm_mday, local_time.tm_hour,
                           local_time.tm_min, local_time.tm_sec);
    } catch (const std::exception& ex) {
        report_exception_error("std::exception", ex);
        return kFallbackTimestamp;
    } catch (...) {
        report_exception_error("std::runtime_error",
                               std::runtime_error("Unknown exception."));
        return kFallbackTimestamp;
    }
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

const std::string DIFFICULTY_STRING = "CNHMGT";
int difficulty_char_to_int(char ch) {
    size_t index = DIFFICULTY_STRING.find(ch);
    return (index != std::string::npos) ? static_cast<int>(index) : -1;
}
char difficulty_int_to_char(int index) {
    if (index < 0 || index >= DIFFICULTY_STRING.size()) {
        throw std::out_of_range("Invalid difficulty index");
    }
    return DIFFICULTY_STRING[index];
}

int hardware_concurrency() {
    return std::thread::hardware_concurrency();
}

std::string format_double_with_precision(double value, int precision) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
    std::string str(buffer);

    size_t dot_pos = str.find('.');
    if (dot_pos != std::string::npos) {
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);
        if (str.back() == '.') {
            str.pop_back();
        }
    }
    return str;
}

uint64_t get_current_time() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
