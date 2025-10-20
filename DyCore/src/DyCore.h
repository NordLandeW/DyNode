#pragma once
#include <windows.h>

#include <filesystem>

HWND get_hwnd_handle();
HMODULE get_hmodule();

std::filesystem::path get_program_path();
