#pragma once
#include <cstdint>
#include <json.hpp>
#include <string>
#include <vector>

#include "utils.h"

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

struct TimingPointExportView {
    const TimingPoint& tp;
    TimingPointExportView(const TimingPoint& tp) : tp(tp) {
    }
};
inline void to_json(nlohmann::json& j, const TimingPointExportView& view) {
    j["offset"] = view.tp.time;
    j["bpm"] = view.tp.get_bpm();
    j["meter"] = view.tp.meter;
}

struct TimingPointImportView {
    TimingPoint& tp;
    TimingPointImportView(TimingPoint& tp) : tp(tp) {
    }
};
inline void from_json(const nlohmann::json& j, TimingPointImportView& view) {
    j.at("offset").get_to(view.tp.time);
    double bpm;
    j.at("bpm").get_to(bpm);
    view.tp.set_bpm(bpm);
    j.at("meter").get_to(view.tp.meter);
}

class TimingManager {
   private:
    std::vector<TimingPoint> timingPoints;
    bool outOfOrder = false;
    uint64_t lastModifiedTime = 0;

    void mark_modified() {
        lastModifiedTime = get_current_time();
    }

   public:
    uint64_t get_last_modified_time() const {
        return lastModifiedTime;
    }

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

    // Get the timing points array.
    void get_timing_points(std::vector<TimingPoint>& outPoints);

    bool has_timing_point_at(double time);

    int count() {
        return timingPoints.size();
    }

    // Dump the timing points array to JSON.
    nlohmann::json dump_json() {
        sort();
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

    void change_timing_point_at_time(double time, const TimingPoint& tp);
    void delete_timing_point_at_time(double time);
    void add_offset(double offset);
};

TimingManager& get_timing_manager();
