#include <string>

#include "utils.h"

#ifdef WIN32

#include "DyCore.h"
#include "gm.h"
#include "imm.h"
#include "window.h"

WNDPROC g_fnOldWndProc = NULL;
HWND g_hMenuBar = NULL;
bool g_isMenuExpanded = true;

LRESULT CALLBACK SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam);
void SetupWindowHooks(HWND targetHwnd);

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

        case WM_SIZE: {
            LRESULT result =
                CallWindowProc(g_fnOldWndProc, hWnd, uMsg, wParam, lParam);

            return result;
        }
        case WM_DESTROY:
        case WM_NCDESTROY: {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)g_fnOldWndProc);
            g_fnOldWndProc = NULL;
            break;
        }
    }

    return CallWindowProc(g_fnOldWndProc, hWnd, uMsg, wParam, lParam);
}

void SetupWindowHooks(HWND targetHwnd) {
    if (!IsWindow(targetHwnd))
        return;

    auto hModule = get_hmodule();

    DragAcceptFiles(targetHwnd, TRUE);
    if (GetWindowLongPtr(targetHwnd, GWLP_WNDPROC) !=
        (LONG_PTR)SubclassWndProc) {
        g_fnOldWndProc = (WNDPROC)SetWindowLongPtr(targetHwnd, GWLP_WNDPROC,
                                                   (LONG_PTR)SubclassWndProc);
        print_debug_message("Window subclassed successfully.");
    }
}

int window_init() {
    SetupWindowHooks(get_hwnd_handle());
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

void disable_ime() {
    print_debug_message("IME disable skipped: not on Windows.");
}

void enable_ime() {
    print_debug_message("IME enable skipped: not on Windows.");
}

#endif