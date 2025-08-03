#include "timing.h"

#include <algorithm>

TimingManager& get_timing_manager() {
    static TimingManager instance;
    return instance;
}

void TimingManager::clear() {
    timingPoints.clear();
}

void TimingManager::add_timing_point(TimingPoint timingPoint) {
    timingPoints.push_back(timingPoint);
    outOfOrder = true;
}

void TimingManager::sort() {
    if (!outOfOrder)
        return;
    outOfOrder = false;
    std::sort(timingPoints.begin(), timingPoints.end(),
              [](const TimingPoint& a, const TimingPoint& b) {
                  return a.time < b.time;
              });
}
