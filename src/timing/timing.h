#pragma once
#include <string>
#include <vector>

#include "json.hpp"

struct TimingPoint {
    double time;
    double beatLength;
    int meter;

    void set_bpm(double bpm) {
        beatLength = 60000.0 / bpm;
    }
    double get_bpm() const {
        return 60000.0 / beatLength;
    }
};
inline void to_json(nlohmann::json& j, const TimingPoint& tp) {
    j["time"] = tp.time;
    j["beatLength"] = tp.beatLength;
    j["meter"] = tp.meter;
}
inline void from_json(const nlohmann::json& j, TimingPoint& tp) {
    j.at("time").get_to(tp.time);
    j.at("beatLength").get_to(tp.beatLength);
    j.at("meter").get_to(tp.meter);
}

class TimingManager {
   private:
    std::vector<TimingPoint> timingPoints;
    bool outOfOrder = false;

   public:
    // Should be called before any operations that require sorted timing points.
    // This will sort the timing points if they are out of order.
    // If they are already sorted, this is a no-op.
    void sort();

    // Clear all timing points.
    void clear();

    // Add a single timing point.
    void add_timing_point(TimingPoint timingPoint);

    // Append multiple timing points.
    void append_timing_points(const std::vector<TimingPoint>& points);

    // Dump the timing points array to JSON.
    nlohmann::json dump_json() {
        return timingPoints;
    }
    // Dump the timing points array to a string.
    std::string dump() {
        return dump_json().dump();
    }

    int size() {
        return timingPoints.size();
    }

    TimingPoint operator[](int index) {
        sort();
        return timingPoints[index];
    }
    TimingPoint operator=(const TimingPoint& other) = delete;
};

TimingManager& get_timing_manager();
