#include "timing.h"

#include <algorithm>

TimingManager& get_timing_manager() {
    static TimingManager instance;
    return instance;
}

void TimingManager::clear() {
    timingPoints.clear();
    mark_modified();
}

void TimingManager::add_timing_point(TimingPoint timingPoint) {
    timingPoints.push_back(timingPoint);
    outOfOrder = true;
    mark_modified();
}

void TimingManager::sort() {
    if (!outOfOrder)
        return;
    outOfOrder = false;
    std::sort(timingPoints.begin(), timingPoints.end(),
              [](const TimingPoint& a, const TimingPoint& b) {
                  return a.time < b.time;
              });
    mark_modified();
}

void TimingManager::append_timing_points(
    const std::vector<TimingPoint>& points) {
    timingPoints.insert(timingPoints.end(), points.begin(), points.end());
    outOfOrder = true;
    mark_modified();
}

void TimingManager::get_timing_points(std::vector<TimingPoint>& outPoints) {
    sort();
    outPoints = timingPoints;
}

void TimingManager::change_timing_point_at_time(double time,
                                                const TimingPoint& tp) {
    for (auto& point : timingPoints) {
        if (point.time == time) {
            point = tp;
            mark_modified();
            return;
        }
    }
}

void TimingManager::delete_timing_point_at_time(double time) {
    timingPoints.erase(std::remove_if(timingPoints.begin(), timingPoints.end(),
                                      [time](const TimingPoint& point) {
                                          return point.time == time;
                                      }),
                       timingPoints.end());
    mark_modified();
}

void TimingManager::add_offset(double offset) {
    for (auto& point : timingPoints) {
        point.time += offset;
    }
    mark_modified();
}
