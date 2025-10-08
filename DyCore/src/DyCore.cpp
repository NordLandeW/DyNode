
#include "DyCore.h"

#include <windef.h>
#include <winuser.h>

#include <cstring>
#include <iostream>

#include "config.h"
#include "tools.h"
#include "utils.h"
#include "version.h"

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

    // Check FFmpeg availability
    if (is_FFmpeg_available()) {
        print_debug_message("FFmpeg is available.");
    }

    print_debug_message("DyCore Initialization finished. No errors.");

    return "success";
}

DYCORE_API const char* DyCore_get_version() {
    return DYNODE_VERSION.c_str();
}

DYCORE_API double DyCore_is_release_build() {
    if (DYNODE_BUILD_TYPE == "RELEASE") {
        return 1.0;
    } else {
        return 0.0;
    }
}

DYCORE_API const char* DyCore_get_goog_measurement_id() {
    return GOOG_MEASUREMENT_ID.c_str();
}

DYCORE_API const char* DyCore_get_goog_api_secret() {
    return GOOG_API_SECRET.c_str();
}