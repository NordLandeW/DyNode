
#include "DyCore.h"

#include <windef.h>
#include <winuser.h>

#include <cstring>
#include <iostream>

#include "analytics.h"
#include "api.h"
#include "config.h"
#include "extension.h"
#include "ffmpeg/base.h"
#include "ffmpeg/record.h"
#include "notePoolManager.h"
#include "profile.h"
#include "project.h"
#include "render.h"
#include "telemetry.h"
#include "utils.h"
#include "version.h"
#include "video/decoder.h"
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

namespace {
bool noteSubsystemInitialized = false;
}

DYCORE_API double DyCore_shutdown();

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

    print_debug_message("-- Program path: " + ::programPath.string());
    print_debug_message("-- Working directory: " +
                        std::filesystem::current_path().string());

    try {
        (void)Profiler::get();
        initialize_project_saves();
        noteSubsystemInitialized = true;
        get_note_pool_manager().initialize_executor();
        initialize_note_rendering();
        if (window_init() != 0) {
            throw std::runtime_error("Failed to install window hook");
        }
        init_analytics();
        print_debug_message("-- DyCore Initialization finished. No errors.");
        return "success";
    } catch (const std::exception& error) {
        print_debug_message(std::string("DyCore initialization failed: ") +
                            error.what());
        DyCore_shutdown();
        try {
            (void)shutdown_telemetry("", "", "[]", 0);
        } catch (...) {
            // Initialization failure must not add an unbounded SDK close.
        }
        return "initfailed";
    }
}

// Drain before GML map_close invalidates identity and clears live note data.
DYCORE_API double DyCore_shutdown_project_saves() {
    try {
        shutdown_project_saves();
        return 0.0;
    } catch (const std::exception& error) {
        print_debug_message(std::string("Project save shutdown failed: ") +
                            error.what());
        return -1.0;
    }
}

// Called from Game End on the owner thread, before DLL unloading.
DYCORE_API double DyCore_shutdown() {
    bool succeeded = true;
    auto cleanup = [&](const char* name, auto action) {
        try {
            action();
        } catch (const std::exception& error) {
            succeeded = false;
            print_debug_message(std::string("DyCore shutdown: ") + name + ": " +
                                error.what());
        } catch (...) {
            succeeded = false;
            print_debug_message(std::string("DyCore shutdown failed: ") + name);
        }
    };
    cleanup("project saves", [] { shutdown_project_saves(); });
    cleanup("Lua", [] { cancel_lua_script(); });
    cleanup("recorder", [] { shutdown_recorder(); });
    cleanup("video", [] { VideoDecoder::shutdown_instance(); });
    cleanup("render executor", [] { shutdown_note_rendering(); });
    cleanup("note executor", [] {
        if (noteSubsystemInitialized) {
            get_note_pool_manager().shutdown_executor();
            noteSubsystemInitialized = false;
        }
    });
    cleanup("window hook", [] {
        const int result = window_shutdown();
        if (result < 0)
            throw std::runtime_error("Failed to restore window procedure");
        if (result > 0) {
            print_debug_message(
                "Window hook is inactive; retained in a foreign chain until "
                "WM_NCDESTROY.");
        }
    });
    return succeeded ? 0.0 : -1.0;
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

DYCORE_API double DyCore_is_debug_build() {
    return DYCORE_DEBUG_BUILD ? 1.0 : 0.0;
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
