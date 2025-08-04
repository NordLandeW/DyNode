
#pragma once
#include <Windows.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

void print_debug_message(std::string str);
fs::path convert_char_to_path(const char* filePath);
std::string wstringToUtf8(const std::wstring& wstr);
std::wstring s2ws(const std::string& str);
std::string gb2312ToUtf8(const std::string& gbStr);
std::string get_file_modification_time(char* file_path);
std::string random_string(int length);
int difficulty_char_to_int(char ch);

int hardware_concurrency();