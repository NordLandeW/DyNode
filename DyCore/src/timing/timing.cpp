#include "timing.h"

#include <algorithm>

TimingManager& get_timing_manager() {
    static TimingManager instance;
    return instance;
}

void TimingManager::clear() {
    std::unique_lock lock(mutex);
    timingPoints.clear();
    mark_modified();
}

void TimingManager::add_timing_point(TimingPoint timingPoint) {
    std::unique_lock lock(mutex);
    timingPoints.push_back(timingPoint);
    outOfOrder = true;
    mark_modified();
}

void TimingManager::sort() {
    read_sorted([](const auto&) {});
}

void TimingManager::sort_points() {
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
    std::unique_lock lock(mutex);
    timingPoints.insert(timingPoints.end(), points.begin(), points.end());
    outOfOrder = true;
    mark_modified();
}

void TimingManager::get_timing_points(std::vector<TimingPoint>& outPoints) {
    read_sorted([&outPoints](const auto& points) { outPoints = points; });
}

const double TIMING_POINT_EPSILON = 1;
bool TimingManager::has_timing_point_at(double time) {
    return read_sorted([time](const auto& points) {
        auto it = std::lower_bound(
            points.begin(), points.end(), time,
            [](const TimingPoint& a, double b) { return a.time < b; });

        // Check the element at the found position (or the one after the target
        // time)
        if (it != points.end()) {
            if (std::abs(it->time - time) < TIMING_POINT_EPSILON) {
                return true;
            }
        }

        // Check the element before the found position (the one before the
        // target time)
        if (it != points.begin()) {
            auto prev_it = std::prev(it);
            if (std::abs(prev_it->time - time) < TIMING_POINT_EPSILON) {
                return true;
            }
        }

        return false;
    });
}

bool TimingManager::get_timing_point_at(double time, TimingPoint& outPoint) {
    return read_sorted([time, &outPoint](const auto& points) {
        if (points.empty()) {
            return false;
        }

        auto it = std::upper_bound(points.begin(), points.end(), time,
                                   [](double value, const TimingPoint& point) {
                                       return value < point.time;
                                   });

        if (it == points.begin()) {
            outPoint = *it;
        } else {
            outPoint = *std::prev(it);
        }
        return true;
    });
}

void TimingManager::change_timing_point_at_time(double time,
                                                const TimingPoint& tp) {
    std::unique_lock lock(mutex);
    for (auto& point : timingPoints) {
        if (point.time == time) {
            point = tp;
            outOfOrder = true;
            mark_modified();
            return;
        }
    }
}

void TimingManager::delete_timing_point_at_time(double time) {
    std::unique_lock lock(mutex);
    timingPoints.erase(std::remove_if(timingPoints.begin(), timingPoints.end(),
                                      [time](const TimingPoint& point) {
                                          return point.time == time;
                                      }),
                       timingPoints.end());
    mark_modified();
}

void TimingManager::add_offset(double offset) {
    std::unique_lock lock(mutex);
    for (auto& point : timingPoints) {
        point.time += offset;
    }
    mark_modified();
}
