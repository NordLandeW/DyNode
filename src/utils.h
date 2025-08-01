
#pragma once
#include <Windows.h>

#include <string>

void print_debug_message(std::string str);
std::string wstringToUtf8(const std::wstring& wstr);
std::wstring s2ws(const std::string& str);
std::string gb2312ToUtf8(const std::string& gbStr);
std::string get_file_modification_time(char* file_path);
std::string random_string(int length);

int hardware_concurrency();