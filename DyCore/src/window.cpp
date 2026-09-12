#include <string>

#include "utils.h"

#ifdef _WIN32

#include "DyCore.h"
#include "gm.h"
#include "imm.h"
#include "window.h"

WNDPROC g_fnOldWndProc = NULL;
HWND g_hMenuBar = NULL;
bool g_isMenuExpanded = true;
HWND g_hookedWindow = NULL;
bool g_hookEventsEnabled = false;
bool g_acceptedDropsBeforeHook = false;

LRESULT CALLBACK SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam);
bool SetupWindowHooks(HWND targetHwnd);

void OnFilesDropped(const std::vector<std::wstring>& files) {
    if (files.empty())
        return;

    std::vector<std::string> utf8Files;
    for (const auto& file : files) {
        utf8Files.push_back(wstringToUtf8(file));
    }

    json j = utf8Files;
    AsyncEvent event = {ON_FILES_DROPPED, 0, j.dump()};
    push_async_event(event);

    print_debug_message("Files dropped event pushed. Filecount: " +
                        std::to_string(utf8Files.size()));
}

LRESULT CALLBACK SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam) {
    const WNDPROC oldProc = g_fnOldWndProc;
    auto forward = [&] {
        return oldProc ? CallWindowProc(oldProc, hWnd, uMsg, wParam, lParam)
                       : DefWindowProc(hWnd, uMsg, wParam, lParam);
    };
    if (!g_hookEventsEnabled && uMsg != WM_NCDESTROY)
        return forward();
    switch (uMsg) {
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
            std::vector<std::wstring> files;

            for (UINT i = 0; i < fileCount; i++) {
                UINT len = DragQueryFileW(hDrop, i, NULL, 0);
                if (len > 0) {
                    std::vector<wchar_t> buf(len + 1);
                    DragQueryFileW(hDrop, i, buf.data(), len + 1);
                    files.push_back(std::wstring(buf.data()));
                }
            }
            DragFinish(hDrop);

            OnFilesDropped(files);
            return 0;
        }

        case WM_NCDESTROY: {
            window_shutdown();
            g_fnOldWndProc = NULL;
            g_hookedWindow = NULL;
            break;
        }
    }

    return forward();
}

bool SetupWindowHooks(HWND targetHwnd) {
    if (!IsWindow(targetHwnd))
        return false;
    if (g_hookedWindow && g_hookedWindow != targetHwnd &&
        window_shutdown() != 0) {
        return false;
    }
    if (g_hookedWindow == targetHwnd && g_fnOldWndProc) {
        g_hookEventsEnabled = true;
        DragAcceptFiles(targetHwnd, TRUE);
        return true;
    }
    SetLastError(0);
    const auto oldProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
        targetHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassWndProc)));
    if (!oldProc)
        return false;
    g_fnOldWndProc = oldProc;
    g_hookedWindow = targetHwnd;
    g_hookEventsEnabled = true;
    g_acceptedDropsBeforeHook =
        (GetWindowLongPtr(targetHwnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES) != 0;
    DragAcceptFiles(targetHwnd, TRUE);
    print_debug_message("Window subclassed successfully.");
    return true;
}

int window_init() {
    return SetupWindowHooks(get_hwnd_handle()) ? 0 : -1;
}

int window_shutdown() {
    g_hookEventsEnabled = false;
    if (!g_hookedWindow || !IsWindow(g_hookedWindow)) {
        g_hookedWindow = NULL;
        g_fnOldWndProc = NULL;
        return 0;
    }
    DragAcceptFiles(g_hookedWindow, g_acceptedDropsBeforeHook);
    const auto current = reinterpret_cast<WNDPROC>(
        GetWindowLongPtr(g_hookedWindow, GWLP_WNDPROC));
    if (current != SubclassWndProc) {
        // Do not overwrite a later subclass. Stay as a pass-through until
        // WM_NCDESTROY, which still forwards through the saved procedure.
        return 1;
    }
    if (g_fnOldWndProc &&
        !SetWindowLongPtr(g_hookedWindow, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(g_fnOldWndProc))) {
        return -1;
    }
    g_fnOldWndProc = NULL;
    g_hookedWindow = NULL;
    return 0;
}

void disable_ime() {
    HWND hwnd = get_hwnd_handle();
    ImmAssociateContext(hwnd, NULL);
    print_debug_message("IME disabled.");
}

void enable_ime() {
    HWND hwnd = get_hwnd_handle();
    ImmAssociateContextEx(hwnd, NULL, IACE_DEFAULT);
    print_debug_message("IME enabled.");
}

#else

int window_init() {
    print_debug_message("Window initialization skipped: not on Windows.");
    return 0;
}

int window_shutdown() {
    return 0;
}

void disable_ime() {
    print_debug_message("IME disable skipped: not on Windows.");
}

void enable_ime() {
    print_debug_message("IME enable skipped: not on Windows.");
}

#endif