#include "api.h"
#include "json.hpp"
#include "timing.h"

DYCORE_API const char* DyCore_get_timing_array_string() {
    static std::string timingArrayString;
    timingArrayString = get_timing_manager().dump();
    return timingArrayString.c_str();
}

DYCORE_API double DyCore_insert_timing_point(const char* timingPointObject) {
    get_timing_manager().add_timing_point(
        nlohmann::json::parse(timingPointObject).get<TimingPoint>());
    return 0;
}

DYCORE_API double DyCore_get_timing_points_count() {
    return get_timing_manager().count();
}

DYCORE_API double DyCore_timing_points_reset() {
    get_timing_manager().clear();
    return 0;
}

DYCORE_API double DyCore_timing_points_sort() {
    get_timing_manager().sort();
    return 0;
}

DYCORE_API double DyCore_timing_points_change(double time,
                                              const char* timingPointObject) {
    get_timing_manager().change_timing_point_at_time(
        time, nlohmann::json::parse(timingPointObject).get<TimingPoint>());
    return 0;
}

DYCORE_API double DyCore_delete_timing_point_at_time(double time) {
    get_timing_manager().delete_timing_point_at_time(time);
    return 0;
}

DYCORE_API double DyCore_timing_points_add_offset(double offset) {
    get_timing_manager().add_offset(offset);
    return 0;
}
