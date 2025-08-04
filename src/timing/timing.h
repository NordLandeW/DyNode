
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
    nlohmann::json to_json() const {
        nlohmann::json j;
        j["time"] = time;
        j["beatLength"] = beatLength;
        j["meter"] = meter;
        return j;
    }
};

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
    std::string dump() {
        sort();
        nlohmann::json j = nlohmann::json::array();
        for (const auto& point : timingPoints) {
            j.push_back(point.to_json());
        }
        return j.dump();
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
