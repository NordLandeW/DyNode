
#include "DyCore.h"

#include <windef.h>
#include <winuser.h>

#include <cstring>
#include <iostream>

#include "api.h"
#include "config.h"
#include "ffmpeg/base.h"
#include "utils.h"
#include "version.h"
#include "window.h"

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
            DisableThreadLibraryCalls(hModule);
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif  // def WIN32

std::filesystem::path programPath;

std::filesystem::path get_program_path() {
    return programPath;
}

// Initializes the DyCore library.
//
// @return "success" on successful initialization.
DYCORE_API const char* DyCore_init(const char* hwnd, const char* programPath) {
    std::ios::sync_with_stdio(false);
    HWND hwndHandle = reinterpret_cast<HWND>(const_cast<char*>(hwnd));
    ::programPath = convert_char_to_path(programPath);

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
    print_debug_message("Program path: " + ::programPath.string());

    // Initialize window functions
    window_init();

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

DYCORE_API const char* DyCore_get_aptabase_app_key() {
    return APTABASE_APP_KEY.c_str();
}