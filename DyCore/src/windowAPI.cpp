#include "api.h"
#include "window.h"

DYCORE_API double DyCore_enable_ime() {
    enable_ime();
    return 0;
}

DYCORE_API double DyCore_disable_ime() {
    disable_ime();
    return 0;
}

DYCORE_API double DyCore_window_set_close_intercept(double enabled) {
    return window_set_close_intercept(enabled != 0);
}

DYCORE_API double DyCore_window_take_close_request() {
    return window_take_close_request();
}

DYCORE_API double DyCore_window_set_visible(double visible) {
    return window_set_visible(visible != 0);
}
