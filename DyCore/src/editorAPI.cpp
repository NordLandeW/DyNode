#include <stdexcept>
#include <string>

#include "api.h"
#include "editor.h"
#include "utils.h"

DYCORE_API double DyCore_gmeditor_set_ready(double flag) {
    gmeditor_set_ready(flag != 0.0);
    return 0.0;
}

DYCORE_API double DyCore_gmeditor_get_ready() {
    return gmeditor_is_ready() ? 1.0 : 0.0;
}

DYCORE_API double DyCore_gmeditor_sync_states(const char* states) {
    try {
        if (states == nullptr) {
            throw std::invalid_argument("states cannot be null");
        }
        gmeditor_sync_states(nlohmann::json::parse(states));
        return 0.0;
    } catch (const std::exception& e) {
        print_debug_message("DyCore_gmeditor_sync_states failed: " +
                            std::string(e.what()));
        return -1.0;
    } catch (...) {
        print_debug_message(
            "DyCore_gmeditor_sync_states failed: unknown error");
        return -1.0;
    }
}
