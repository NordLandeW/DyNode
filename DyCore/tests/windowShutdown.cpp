#include <doctest/doctest.h>

#ifdef _WIN32

#include <windows.h>

#include <stdexcept>

#include "window.h"

bool SetupWindowHooks(HWND targetHwnd);

namespace {
struct WindowEvents {
    int destroy = 0;
    int nonClientDestroy = 0;
};

LRESULT CALLBACK original_proc(HWND window, UINT message, WPARAM wp,
                               LPARAM lp) {
    auto* events = reinterpret_cast<WindowEvents*>(
        GetWindowLongPtr(window, GWLP_USERDATA));
    if (events) {
        if (message == WM_DESTROY)
            ++events->destroy;
        if (message == WM_NCDESTROY)
            ++events->nonClientDestroy;
        if (message == WM_APP + 7)
            return 1234;
    }
    return DefWindowProc(window, message, wp, lp);
}

WNDPROC foreignPrevious = nullptr;
LRESULT CALLBACK foreign_proc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    return CallWindowProc(foreignPrevious, window, message, wp, lp);
}

struct TestWindow {
    WindowEvents events;
    HINSTANCE module = GetModuleHandle(nullptr);
    HWND window = nullptr;
    static constexpr const wchar_t* CLASS_NAME =
        L"DyNodeShutdownMessageOnlyWindow";

    TestWindow() {
        WNDCLASSW cls{};
        cls.hInstance = module;
        cls.lpfnWndProc = original_proc;
        cls.lpszClassName = CLASS_NAME;
        if (!RegisterClassW(&cls))
            throw std::runtime_error("RegisterClass failed");
        // Message-only: never creates a visible desktop window.
        window = CreateWindowExW(0, CLASS_NAME, L"", 0, 0, 0, 0, 0,
                                 HWND_MESSAGE, nullptr, module, nullptr);
        if (!window) {
            UnregisterClassW(CLASS_NAME, module);
            throw std::runtime_error("CreateWindowEx failed");
        }
        SetWindowLongPtr(window, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(&events));
    }
    ~TestWindow() {
        if (IsWindow(window))
            DestroyWindow(window);
        window_shutdown();
        UnregisterClassW(CLASS_NAME, module);
    }
};
}  // namespace

TEST_CASE("WindowHookForwardsBothDestructionMessagesAndUnhooksIdempotently") {
    TestWindow fixture;
    REQUIRE(SetupWindowHooks(fixture.window));
    CHECK(SendMessage(fixture.window, WM_APP + 7, 0, 0) == 1234);
    CHECK(window_shutdown() == 0);
    CHECK(window_shutdown() == 0);
    CHECK(SendMessage(fixture.window, WM_APP + 7, 0, 0) == 1234);
    REQUIRE(SetupWindowHooks(fixture.window));
    REQUIRE(DestroyWindow(fixture.window));
    CHECK(fixture.events.destroy == 1);
    CHECK(fixture.events.nonClientDestroy == 1);
    CHECK(window_shutdown() == 0);
}

TEST_CASE("WindowShutdownPreservesAForeignSubclassChain") {
    TestWindow fixture;
    REQUIRE(SetupWindowHooks(fixture.window));
    foreignPrevious = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(fixture.window, GWLP_WNDPROC,
                         reinterpret_cast<LONG_PTR>(foreign_proc)));
    REQUIRE(foreignPrevious != nullptr);
    CHECK(window_shutdown() == 1);
    CHECK(reinterpret_cast<WNDPROC>(
              GetWindowLongPtr(fixture.window, GWLP_WNDPROC)) == foreign_proc);
    CHECK(SendMessage(fixture.window, WM_APP + 7, 0, 0) == 1234);
    REQUIRE(DestroyWindow(fixture.window));
    CHECK(fixture.events.destroy == 1);
    CHECK(fixture.events.nonClientDestroy == 1);
}
#endif
