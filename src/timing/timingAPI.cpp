#include "api.h"
#include "timing.h"

DYCORE_API const char* DyCore_get_timing_array_string() {
    static std::string timingArrayString;
    timingArrayString = get_timing_manager().dump();
    return timingArrayString.c_str();
}