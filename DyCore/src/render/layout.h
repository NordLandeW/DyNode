#pragma once

#include <cmath>

inline constexpr int BASE_RES_W = 1920;
inline constexpr int BASE_RES_H = 1080;

inline constexpr int JUDGE_LINE_BELOW_FROM_BOTTOM = 137;
inline constexpr int JUDGE_LINE_SIDE_FROM_EDGE = 112;

inline constexpr int ACTIVATION_AHEAD_PIXELS = 50;

struct PlaybackFrame {
    double time, frontSpeed, sideSpeed;
    bool dynamite;

    double speed(int side) const {
        return side == 0 ? frontSpeed : sideSpeed;
    }
};

inline constexpr double DYNAMITE_WORLD_PX = BASE_RES_H / 16.0;
inline constexpr double DYNAMITE_TEXEL = DYNAMITE_WORLD_PX / 75.0;

inline double dynamite_min_duration(double speed, int side) {
    const double unit = DYNAMITE_WORLD_PX * 0.025 * (side == 0 ? 1.0 : 0.9);
    return speed > 0.0 ? std::floor(16.0 * unit / speed) : 0.0;
}
