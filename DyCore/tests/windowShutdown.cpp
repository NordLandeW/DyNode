#include <doctest/doctest.h>

#include "window.h"

#ifdef _WIN32

#include <windows.h>

#include <stdexcept>

bool SetupWindowHooks(HWND targetHwnd);

namespace {
struct WindowEvents {
    int destroy = 0;
    int nonClientDestroy = 0;
    int close = 0;
    int systemClose = 0;
    int eraseBackground = 0;
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
        if (message == WM_CLOSE)
            ++events->close;
        if (message == WM_SYSCOMMAND) {
            if ((wp & 0xFFF0) == SC_CLOSE)
                ++events->systemClose;
            else
                return 4321;
        }
        if (message == WM_ERASEBKGND) {
            ++events->eraseBackground;
            return 57;
        }
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

TEST_CASE("WindowCloseRequestsAreInterceptedCoalescedAndConsumed") {
    TestWindow fixture;
    REQUIRE(SetupWindowHooks(fixture.window));
    REQUIRE(window_set_close_intercept(true) == 0);
    CHECK(window_take_close_request() == 0);

    SUBCASE("System close masks the reserved low bits") {
        SendMessage(fixture.window, WM_SYSCOMMAND, SC_CLOSE | 0x000F, 0);
    }
    SUBCASE("Direct close") {
        SendMessage(fixture.window, WM_CLOSE, 0, 0);
    }
    REQUIRE(IsWindow(fixture.window));
    REQUIRE(SetupWindowHooks(fixture.window));
    CHECK(window_set_close_intercept(true) == 0);
    CHECK(window_take_close_request() == 1);
    CHECK(window_take_close_request() == 0);

    // Keeping interception enabled after consuming a request cancels closing.
    SendMessage(fixture.window, WM_CLOSE, 0, 0);
    SendMessage(fixture.window, WM_SYSCOMMAND, SC_CLOSE, 0);
    CHECK(window_take_close_request() == 1);
    CHECK(window_take_close_request() == 0);
    CHECK(fixture.events.close == 0);
    CHECK(fixture.events.systemClose == 0);
    CHECK(fixture.events.destroy == 0);
}

TEST_CASE("WindowApprovedCloseReachesTheOriginalProcedure") {
    TestWindow fixture;
    REQUIRE(SetupWindowHooks(fixture.window));
    REQUIRE(window_set_close_intercept(true) == 0);
    SendMessage(fixture.window, WM_CLOSE, 0, 0);
    REQUIRE(window_set_close_intercept(false) == 0);
    CHECK(window_take_close_request() == 0);

    SUBCASE("System close") {
        SendMessage(fixture.window, WM_SYSCOMMAND, SC_CLOSE, 0);
        CHECK(fixture.events.systemClose == 1);
    }
    SUBCASE("Direct close") {
        SendMessage(fixture.window, WM_CLOSE, 0, 0);
    }
    CHECK(fixture.events.close == 1);
    CHECK(fixture.events.destroy == 1);
    CHECK(fixture.events.nonClientDestroy == 1);
    CHECK_FALSE(IsWindow(fixture.window));
    CHECK(window_take_close_request() == -1);
}

TEST_CASE("WindowCloseHookPreservesBackgroundErasureAndOtherMessages") {
    TestWindow fixture;
    REQUIRE(SetupWindowHooks(fixture.window));
    REQUIRE(window_set_close_intercept(true) == 0);
    CHECK(SendMessage(fixture.window, WM_ERASEBKGND, 0, 0) == 57);
    CHECK(fixture.events.eraseBackground == 1);
    CHECK(SendMessage(fixture.window, WM_SYSCOMMAND, SC_RESTORE, 0) == 4321);
    CHECK(SendMessage(fixture.window, WM_APP + 7, 0, 0) == 1234);
    CHECK(window_take_close_request() == 0);
}

TEST_CASE("WindowCloseStateIsClearedOnShutdownAndReinitialization") {
    TestWindow fixture;
    REQUIRE(SetupWindowHooks(fixture.window));
    REQUIRE(window_set_close_intercept(true) == 0);
    SendMessage(fixture.window, WM_CLOSE, 0, 0);
    REQUIRE(window_shutdown() == 0);
    CHECK(window_set_close_intercept(true) == -1);
    CHECK(window_take_close_request() == -1);
    CHECK(window_set_visible(false) == -1);
    REQUIRE(SetupWindowHooks(fixture.window));
    CHECK(window_take_close_request() == 0);
    // Reinitialization starts with interception disabled.
    SendMessage(fixture.window, WM_CLOSE, 0, 0);
    CHECK(fixture.events.close == 1);
    CHECK(fixture.events.destroy == 1);
}

TEST_CASE("WindowVisibilityChangesPreserveTheWindow") {
    TestWindow fixture;
    REQUIRE(SetupWindowHooks(fixture.window));
    // A message-only window cannot appear on the desktop, even when shown.
    CHECK(window_set_visible(true) == 0);
    CHECK((GetWindowLongPtr(fixture.window, GWL_STYLE) & WS_VISIBLE) != 0);
    CHECK(window_set_visible(false) == 0);
    CHECK((GetWindowLongPtr(fixture.window, GWL_STYLE) & WS_VISIBLE) == 0);
    CHECK(window_set_visible(false) == 0);
    CHECK(IsWindow(fixture.window));
    CHECK(fixture.events.destroy == 0);
}

#else

TEST_CASE("WindowOperationsReportNotImplemented") {
    CHECK(window_set_close_intercept(true) == WINDOW_NOT_IMPLEMENTED);
    CHECK(window_set_close_intercept(false) == WINDOW_NOT_IMPLEMENTED);
    CHECK(window_take_close_request() == WINDOW_NOT_IMPLEMENTED);
    CHECK(window_set_visible(true) == WINDOW_NOT_IMPLEMENTED);
    CHECK(window_set_visible(false) == WINDOW_NOT_IMPLEMENTED);
}

#endif
