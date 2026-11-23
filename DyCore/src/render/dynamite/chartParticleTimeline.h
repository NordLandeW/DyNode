#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

// Sorted judgments and compact Hold intervals on the 60 Hz playback clock.
class ChartParticleTimeline {
   public:
    using Emit =
        std::function<void(int64_t tick, int side, float centerPx,
                           double chartWidth, bool sustained, uint64_t seed)>;
    bool refresh();
    size_t capacity_bytes() const;
    void visit(int64_t firstTick, int64_t lastTick, const Emit& emit) const;

   private:
    struct Builder;
    struct Event {
        int64_t tick;
        double width;
        uint64_t seed;
        int side;
        float center;
    };
    struct Hold {
        Event event;
        int64_t end, subtreeEnd;
    };
    int64_t index_holds(size_t first, size_t last);
    bool visit_holds(size_t first, size_t last, int64_t tick,
                     const Emit& emit) const;
    uint64_t revision = 0;
    std::vector<Event> bursts;
    std::vector<Hold> holds;
};
