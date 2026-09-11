#include <cmath>
#include <limits>
#include <stdexcept>

#include "api.h"
#include "json.hpp"
#include "timing.h"

namespace {
TimingPoint parse_valid_timing_point(const char* object) {
    if (!object) {
        throw std::invalid_argument("Timing point is null.");
    }
    const auto value = nlohmann::json::parse(object);
    const double meter = value.at("meter").get<double>();
    if (!std::isfinite(meter) || meter < 1 ||
        meter > (std::numeric_limits<int>::max)() ||
        std::floor(meter) != meter) {
        throw std::invalid_argument(
            "Timing meter must be a positive representable integer.");
    }
    const auto point = value.get<TimingPoint>();
    if (!std::isfinite(point.time) || !std::isfinite(point.beatLength) ||
        point.beatLength <= 0) {
        throw std::invalid_argument(
            "Timing offset must be finite and beat length must be finite and "
            "positive.");
    }
    return point;
}
}  // namespace

DYCORE_API const char* DyCore_get_timing_array_string() {
    static std::string timingArrayString;
    timingArrayString = get_timing_manager().dump();
    return timingArrayString.c_str();
}

DYCORE_API const char* DyCore_get_timing_point_at(double time) {
    static std::string timingPointString;
    TimingPoint timingPoint;
    if (!get_timing_manager().get_timing_point_at(time, timingPoint)) {
        timingPointString.clear();
        return timingPointString.c_str();
    }

    timingPointString = nlohmann::json(timingPoint).dump();
    return timingPointString.c_str();
}

DYCORE_API double DyCore_insert_timing_point(const char* timingPointObject) {
    try {
        get_timing_manager().add_timing_point(
            parse_valid_timing_point(timingPointObject));
        return 0;
    } catch (const std::exception& e) {
        print_debug_message(std::string("Invalid timing point: ") + e.what());
        return -1;
    }
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
    try {
        if (!std::isfinite(time)) {
            return -1;
        }
        get_timing_manager().change_timing_point_at_time(
            time, parse_valid_timing_point(timingPointObject));
        return 0;
    } catch (const std::exception& e) {
        print_debug_message(std::string("Invalid timing point: ") + e.what());
        return -1;
    }
}

DYCORE_API double DyCore_delete_timing_point_at_time(double time) {
    get_timing_manager().delete_timing_point_at_time(time);
    return 0;
}

DYCORE_API double DyCore_timing_points_add_offset(double offset) {
    get_timing_manager().add_offset(offset);
    return 0;
}

DYCORE_API double DyCore_get_timing_points_last_modified_time() {
    return get_timing_manager().get_last_modified_time();
}

DYCORE_API double DyCore_has_timing_point_at_time(double time) {
    return get_timing_manager().has_timing_point_at(time) ? 1 : 0;
}
