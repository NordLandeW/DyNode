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