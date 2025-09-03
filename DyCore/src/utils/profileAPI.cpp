#include <sstream>

#include "api.h"
#include "profile.h"

DYCORE_API const char* DyCore_profile_report() {
    static std::string result;
    std::ostringstream oss;
    Profiler::get().generate_report(oss);
    result = oss.str();
    return result.c_str();
}