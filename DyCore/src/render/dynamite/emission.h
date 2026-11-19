#pragma once

#include <algorithm>
#include <limits>

namespace dynamite_emission {
inline constexpr double TICKS_PER_MS = 60.0 / 1000;
inline constexpr double TICK_EPSILON = 1e-9;
inline float emission_width(int side, double width) {
    return static_cast<float>(
        std::max(2.8, 5.6 * width * (side == 0 ? 1 : .48)));
}
inline int burst_amount(float width) {
    return static_cast<int>(
        std::min<double>(3.5f * width, std::numeric_limits<int>::max()));
}
}  // namespace dynamite_emission
