#include <exception>
#include <string>

#include "api.h"
#include "dlgbox.h"
#include "gm.h"
#include "utils.h"

DYCORE_API const char* DyCore_get_save_filename(const char* filter,
                                                const char* default_filename,
                                                const char* initial_directory,
                                                const char* caption) {
    static std::string strResult;
    try {
        auto result =
            get_save_filename(filter, default_filename,
                              convert_char_to_path(initial_directory), caption);
        strResult =
            result.has_value() ? wstringToUtf8(result->c_str()) : "terminated";
    } catch (const std::exception& e) {
        throw_error_event(e.what());
        return "";
    }
    return strResult.c_str();
}

DYCORE_API const char* DyCore_get_open_filename(const char* filter,
                                                const char* default_filename,
                                                const char* initial_directory,
                                                const char* caption) {
    static std::string strResult;
    try {
        auto result =
            get_open_filename(filter, default_filename,
                              convert_char_to_path(initial_directory), caption);
        strResult =
            result.has_value() ? wstringToUtf8(result->c_str()) : "terminated";
    } catch (const std::exception& e) {
        throw_error_event(e.what());
        return "";
    }
    return strResult.c_str();
}

DYCORE_API double DyCore_show_question(const char* question_text) {
    return show_question(question_text) ? 1.0 : 0.0;
}

DYCORE_API const char* DyCore_get_string(const char* prompt,
                                         const char* default_text) {
    static string result;
    try {
        result = get_string(prompt, default_text)
                     .value_or("<%$><.3>TERMINATED<!#><##>");
    } catch (std::exception& e) {
        throw_error_event(e.what());
        return "";
    }

    return result.c_str();
}