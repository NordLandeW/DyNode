
#include "DyCore.h"

#include <windef.h>
#include <winuser.h>

#include <cstring>
#include <iostream>

#include "utils.h"

#ifdef WIN32
#include <windows.h>
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

HWND hwndParent = NULL;
HMODULE hModule = NULL;

HWND get_hwnd_handle() {
    return hwndParent;
}
HMODULE get_hmodule() {
    return hModule;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            ::hModule = hModule;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif  // def WIN32

// Initializes the DyCore library.
//
// @return "success" on successful initialization.
DYCORE_API const char* DyCore_init(const char* hwnd) {
    std::ios::sync_with_stdio(false);
    HWND hwndHandle = reinterpret_cast<HWND>(const_cast<char*>(hwnd));

    // Check hwndHandle
    if (hwndHandle == NULL) {
        return "hwndfailed";
    }
    if (!IsWindow(hwndHandle)) {
        return "hwndfailed";
    }

    hwndParent = hwndHandle;

    print_debug_message("DyCore Initialization finished. No errors.");

    return "success";
}
