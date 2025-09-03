#pragma once
#include <windows.h>

#include "api.h"

HWND get_hwnd_handle();
HMODULE get_hmodule();
DYCORE_API const char* DyCore_init(const char* hwnd);
