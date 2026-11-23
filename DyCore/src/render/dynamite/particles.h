#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <glm/glm.hpp>
#include <optional>
#include <random>
#include <vector>

#include "chartParticleTimeline.h"

// Shared HEPInner emitter and draw path; only the source of births and time
// differs between actual editor judgments and chart-locked playback.
class DynamiteParticles {
   public:
    explicit DynamiteParticles(uint64_t seed = std::random_device{}())
        : randomState(seed) {
    }
    void reset();
    void queue_hit(int side, double center, double width, double clock,
                   bool mixer, double chartStart = 0, double duration = 0);
    void update_editor(double time, double clock, bool playing);
    void sample_chart(double time);
    size_t count() const;
    size_t capacity_bytes() const;
    size_t render(char* out) const;

   private:
    struct Particle {
        glm::vec2 origin, velocity;
        double birth;
        float lifetime, size;
    };
    struct Emitter {
        int side;
        float center, radius;
    };
    struct Hold {
        Emitter emitter;
        double chartEnd;
        int64_t firstTick;
    };
    struct Burst {
        Emitter emitter;
        int64_t tick;
        int amount;
        bool mixer;
    };
    struct Admission {
        int64_t tick;
        int amount;
    };
    float range(float low, float high);
    bool has_room(int side, double birth) const;
    void emit(const Emitter& e, double birth, glm::vec2 position,
              glm::vec2 velocity);
    void burst(const Burst& b, double birth);
    void sustain(const Emitter& e, double birth);
    void expire(double clock);
    void consume_editor_bursts(int64_t tick, double clock, bool playing);
    void sustain_editor_holds(int64_t tick, double birth, double time,
                              double clock);
    std::array<std::deque<Particle>, 3> lanes;
    std::deque<Burst> pending;
    std::vector<Hold> holds;
    std::deque<Admission> admitted;
    std::optional<ChartParticleTimeline> chart;
    int64_t admittedCount = 0;
    int64_t lastTick = -1;
    double drawClock = -1, lastChartTime = 0;
    bool wasPlaying = false;
    uint64_t randomState;
};
