#pragma once
#include <string>

enum class IMPORT_DY_RESULT_STATES { SUCCESS, FAILURE, OLD_FORMAT };
std::string get_dy_remix();
IMPORT_DY_RESULT_STATES chart_import_dy(const char* filePath, bool importInfo,
                                        bool importTiming);