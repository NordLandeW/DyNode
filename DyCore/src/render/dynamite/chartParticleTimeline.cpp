#include "chartParticleTimeline.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <compare>
#include <limits>
#include <utility>

#include "emission.h"
#include "notePoolManager.h"
#include "render.h"

namespace {
struct Source {
    double time;
    int side;
    double position, width;
    NOTE_TYPE type;
    double end;
    auto operator<=>(const Source&) const = default;
};

bool first_tick(double millis, int64_t& tick) {
    double rounded = std::ceil(millis * dynamite_emission::TICKS_PER_MS -
                               dynamite_emission::TICK_EPSILON);
    if (!(rounded >= -0x1p63 && rounded < 0x1p63))
        return false;
    tick = static_cast<int64_t>(rounded);
    return true;
}

uint64_t seed_for(const Source& n, uint64_t duplicate) {
    auto bits = [](double v) {
        return std::bit_cast<uint64_t>(v == 0 ? 0.0 : v);
    };
    const std::array<uint64_t, 7> key{
        bits(n.time),  static_cast<uint64_t>(n.side), bits(n.position),
        bits(n.width), static_cast<uint64_t>(n.type), bits(n.end),
        duplicate};
    return XXH64(key.data(), sizeof(key), 0);
}
}  // namespace

bool ChartParticleTimeline::refresh() {
    // Only equal chart times need a secondary content ordering. The note pool
    // already owns a chronological index and the authoritative head/tail map.
    ChartParticleTimeline next;
    std::vector<Source> group;
    struct Admission {
        int64_t tick, amount;
    };
    // At most one record per emission tick in the preceding half-second.
    std::vector<Admission> admitted;
    int64_t nominal = 0, tick = 0;
    double groupTime = -std::numeric_limits<double>::infinity();
    auto flush = [&] {
        std::sort(group.begin(), group.end());
        uint64_t duplicate = 0;
        for (size_t i = 0; i < group.size(); ++i) {
            const auto& n = group[i];
            duplicate = i && n == group[i - 1] ? duplicate + 1 : 0;
            const bool hold = n.type == NOTE_TYPE::HOLD;
            if (!hold && nominal > 250)
                continue;
            const double projected = pos_to_horzPos(n.position, n.side);
            if (std::abs(projected) > std::numeric_limits<float>::max())
                continue;
            const float center = static_cast<float>(projected);
            if (hold) {
                int64_t end;
                if (first_tick(n.end, end) && tick < end)
                    next.holds.push_back(
                        {{tick, n.width, seed_for(n, duplicate), n.side,
                          center},
                         end,
                         end});
                continue;
            }
            int coincident = 0;
            for (auto b = next.bursts.rbegin();
                 b != next.bursts.rend() && b->tick == tick; ++b)
                if (b->side == n.side && b->center == center &&
                    ++coincident == 5)
                    break;
            if (coincident == 5)
                continue;
            const int amount = dynamite_emission::burst_amount(
                dynamite_emission::emission_width(n.side, n.width));
            if (!admitted.empty() && admitted.back().tick == tick)
                admitted.back().amount += static_cast<int64_t>(amount);
            else
                admitted.push_back({tick, amount});
            nominal += amount;
            next.bursts.push_back(
                {tick, n.width, seed_for(n, duplicate), n.side, center});
        }
        group.clear();
    };
    bool validTick = false;
    auto visit = [&](const Note& n, double end) {
        const auto type = n.get_note_type();
        if (n.time != groupTime) {
            flush();
            groupTime = n.time;
            validTick = first_tick(n.time, tick);
            if (validTick) {
                auto live = admitted.begin();
                while (live != admitted.end() &&
                       static_cast<uint64_t>(tick) -
                               static_cast<uint64_t>(live->tick) >
                           30) {
                    nominal -= live->amount;
                    ++live;
                }
                admitted.erase(admitted.begin(), live);
            }
        }
        if (!validTick || (nominal > 250 && type != NOTE_TYPE::HOLD) ||
            n.side < 0 || n.side > 2 || !std::isfinite(n.width) ||
            !std::isfinite(n.position) || !std::isfinite(end) ||
            type < NOTE_TYPE::NORMAL || type > NOTE_TYPE::SUB)
            return;
        group.push_back({n.time, n.side, n.position, n.width, type, end});
    };
    // Keep the unchanged-frame read allocation-free even with a large visitor.
    const auto current =
        get_note_pool_manager().read_judgments_since(revision, std::ref(visit));
    if (current == revision)
        return false;
    flush();
    next.revision = current;
    next.index_holds(0, next.holds.size());
    *this = std::move(next);
    return true;
}

int64_t ChartParticleTimeline::index_holds(size_t first, size_t last) {
    if (first == last)
        return std::numeric_limits<int64_t>::min();
    size_t mid = first + (last - first) / 2;
    return holds[mid].subtreeEnd =
               std::max({holds[mid].end, index_holds(first, mid),
                         index_holds(mid + 1, last)});
}

bool ChartParticleTimeline::visit_holds(size_t first, size_t last, int64_t tick,
                                        const Emit& emit) const {
    if (first == last || holds[first].event.tick > tick)
        return false;
    size_t mid = first + (last - first) / 2;
    const auto& h = holds[mid];
    if (h.subtreeEnd <= tick)
        return false;
    bool active = visit_holds(first, mid, tick, emit);
    if (h.event.tick > tick)
        return active;
    if (tick < h.end) {
        emit(tick, h.event.side, h.event.center, h.event.width, true,
             XXH64(&tick, sizeof(tick), h.event.seed));
        active = true;
    }
    return visit_holds(mid + 1, last, tick, emit) || active;
}

void ChartParticleTimeline::visit(int64_t firstTick, int64_t lastTick,
                                  const Emit& emit) const {
    auto burst =
        std::lower_bound(bursts.begin(), bursts.end(), firstTick,
                         [](const Event& e, int64_t t) { return e.tick < t; });
    auto tick = firstTick;
    while (tick <= lastTick) {
        while (burst != bursts.end() && burst->tick == tick) {
            emit(tick, burst->side, burst->center, burst->width, false,
                 burst->seed);
            ++burst;
        }
        bool active = visit_holds(0, holds.size(), tick, emit);
        if (tick == lastTick)
            break;
        if (active) {
            ++tick;
            continue;
        }
        auto hold = std::upper_bound(
            holds.begin(), holds.end(), tick,
            [](int64_t t, const Hold& h) { return t < h.event.tick; });
        if (burst == bursts.end() && hold == holds.end())
            break;
        tick = std::min(burst == bursts.end() ? lastTick : burst->tick,
                        hold == holds.end() ? lastTick : hold->event.tick);
    }
}

size_t ChartParticleTimeline::capacity_bytes() const {
    return bursts.capacity() * sizeof(Event) + holds.capacity() * sizeof(Hold);
}
