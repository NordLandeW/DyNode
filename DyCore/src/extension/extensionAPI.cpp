#include <exception>
#include <string>

#include "api.h"
#include "extension.h"
#include "gm.h"
#include "utils.h"

DYCORE_API double DyCore_run_lua_script() {
    try {
        run_lua_script();
    } catch (const std::exception& e) {
        const std::string errorMessage =
            "DyCore_run_lua_script failed: " + std::string(e.what());
        print_debug_message(errorMessage);
        throw_error_event(errorMessage);
        return -1.0;
    } catch (...) {
        constexpr const char* errorMessage =
            "DyCore_run_lua_script failed: unknown error";
        print_debug_message(errorMessage);
        throw_error_event(errorMessage);
        return -1.0;
    }

    return 0.0;
}
