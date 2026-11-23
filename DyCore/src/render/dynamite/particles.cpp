#include "particles.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include "emission.h"
#include "render.h"
#include "vertex.h"

using namespace dynamite_emission;

namespace {
constexpr float WORLD = DYNAMITE_WORLD_PX;
constexpr size_t MAX_PER_LANE = 300, PARTICLE_BUDGET = 500;
// Both side emitters map local +X up the screen, local +Y to the left.
glm::vec2 project(int side, glm::vec2 local) {
    return (side == 0 ? glm::vec2(local.x, -local.y)
                      : glm::vec2(-local.y, -local.x)) *
           WORLD;
}
}  // namespace

void DynamiteParticles::reset() {
    for (auto& lane : lanes)
        lane.clear();
    pending.clear();
    holds.clear();
    admitted.clear();
    chart.reset();
    admittedCount = 0;
    lastTick = -1;
    drawClock = -1;
    wasPlaying = false;
}

float DynamiteParticles::range(float low, float high) {
    // SplitMix64: an event has a small, independently addressable random
    // stream.
    uint64_t z = (randomState += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    const float u =
        static_cast<float>((z ^ (z >> 31)) >> 40) * (1.0f / 16777216);
    return std::lerp(low, high, u);
}

void DynamiteParticles::queue_hit(int side, double center, double width,
                                  double clock, bool mixer, double chartStart,
                                  double duration) {
    if (side < 0 || side > 2 || !std::isfinite(center) ||
        !std::isfinite(width) || !std::isfinite(clock))
        return;
    if (chart)
        reset();
    const auto tick = std::max(
        lastTick + 1,
        static_cast<int64_t>(std::ceil(clock * TICKS_PER_MS - TICK_EPSILON)));
    const float w = mixer ? 3.6f : emission_width(side, width);
    const Emitter e{side, static_cast<float>(center), .45f * w};
    if (duration > 0)
        holds.push_back({e, chartStart + duration, tick});
    // The source admits the whole nominal burst when the preceding half-second
    // counter is <= 250, even if this burst takes it over the threshold.
    while (!admitted.empty() && admitted.front().tick < tick - 30) {
        admittedCount -= admitted.front().amount;
        admitted.pop_front();
    }
    if (admittedCount > 250)
        return;
    // HitEffect coalesces coincident ordinary judgments after five per update.
    if (!mixer &&
        std::count_if(pending.begin(), pending.end(), [&](const Burst& b) {
            return b.tick == tick && !b.mixer && b.emitter.side == side &&
                   b.emitter.center == static_cast<float>(center);
        }) >= 5)
        return;
    const int n = burst_amount(w);
    pending.push_back({e, tick, n, mixer});
    admitted.push_back({tick, n});
    admittedCount += n;
}

bool DynamiteParticles::has_room(int side, double birth) const {
    const auto& lane = lanes[side];
    if (lane.size() >= MAX_PER_LANE)
        return lane.front().birth < birth;
    return count() < PARTICLE_BUDGET ||
           std::any_of(
               lanes.begin(), lanes.end(),
               [birth](const auto& l) {
                   return !l.empty() && l.front().birth < birth;
               });
}

void DynamiteParticles::emit(const Emitter& e, double birth, glm::vec2 position,
                             glm::vec2 velocity) {
    if (!has_room(e.side, birth))
        return;
    auto& lane = lanes[e.side];
    if (lane.size() >= MAX_PER_LANE)
        lane.pop_front();
    else if (count() >= PARTICLE_BUDGET) {
        const auto oldest = std::min_element(
            lanes.begin(), lanes.end(), [](const auto& a, const auto& b) {
                return !a.empty() &&
                       (b.empty() || a.front().birth < b.front().birth);
            });
        oldest->pop_front();
    }
    const glm::vec2 center =
        e.side == 0
            ? glm::vec2(e.center, BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM)
            : glm::vec2(e.side == 1 ? JUDGE_LINE_SIDE_FROM_EDGE
                                    : BASE_RES_W - JUDGE_LINE_SIDE_FROM_EDGE,
                        e.center);
    lane.push_back({center + project(e.side, position),
                    project(e.side, velocity), birth, range(250, 400),
                    WORLD * range(.8f, 1.1f)});
}

void DynamiteParticles::burst(const Burst& b, double birth) {
    const auto& e = b.emitter;
    const float a = e.radius;
    for (int i = 0; i < b.amount && has_room(e.side, birth); i += 2) {
        const float theta = range(0, 2 * std::numbers::pi_v<float>);
        const float x = std::cos(theta), y = std::sin(theta);
        glm::vec2 ray;
        if (std::abs(y) <= .001f)
            ray = {a * x, 0};
        else if (std::abs(x) <= .001f)
            ray = {0, a * y};
        else
            ray = glm::vec2(x, y) / std::hypot(x / a, y / .9f);
        const glm::vec2 ellipse(a * x, .9f * y);
        for (const auto& offset : {ray, ellipse}) {
            emit(e, birth, offset, glm::normalize(offset) * range(5, 15));
        }
    }
}

void DynamiteParticles::sustain(const Emitter& e, double birth) {
    if (range(0, 1) < .1f)
        emit(e, birth, {e.radius, 0}, {range(0, 4), 0});
    if (range(0, 1) < .1f)
        emit(e, birth, {-e.radius, 0}, {range(-4, 0), 0});
    for (int i = 0; i < 2; ++i) {
        const float x = range(-e.radius, e.radius);
        const float sign = range(0, 1) < .5f ? -1 : 1;
        emit(e, birth, {x, 0}, {range(-1, 1), sign * range(5, 9)});
    }
}

void DynamiteParticles::expire(double clock) {
    for (auto& lane : lanes) {
        std::erase_if(lane, [clock](const Particle& p) {
            return clock - p.birth >= p.lifetime;
        });
    }
}

void DynamiteParticles::consume_editor_bursts(int64_t tick, double clock,
                                              bool playing) {
    while (!pending.empty() && pending.front().tick <= tick) {
        const auto& b = pending.front();
        const double queued = b.tick / TICKS_PER_MS;
        if (playing && clock - queued < 400)
            burst(b, queued);
        pending.pop_front();
    }
}

void DynamiteParticles::sustain_editor_holds(int64_t tick, double birth,
                                             double time, double clock) {
    const double at =
        wasPlaying && drawClock >= 0 && clock > drawClock &&
                time >= lastChartTime
            ? std::lerp(lastChartTime, time,
                        std::clamp((birth - drawClock) / (clock - drawClock),
                                   0.0, 1.0))
            : time;
    for (const auto& h : holds) {
        if (tick >= h.firstTick && at < h.chartEnd &&
            has_room(h.emitter.side, birth))
            sustain(h.emitter, birth);
    }
}

void DynamiteParticles::update_editor(double time, double clock, bool playing) {
    if (chart || clock < drawClock)
        reset();
    const int64_t now =
        static_cast<int64_t>(std::floor(clock * TICKS_PER_MS + TICK_EPSILON));
    // No historical chart replay on entry or resume. After a long stall only
    // the last 400 ms can still contribute a live particle.
    int64_t first =
        lastTick < 0 || !wasPlaying ? now : std::max(lastTick + 1, now - 24);
    first = std::max(first, lastTick + 1);
    for (auto tick = first; tick <= now; ++tick) {
        const double birth = tick / TICKS_PER_MS;
        expire(birth);
        consume_editor_bursts(tick, clock, playing);
        if (playing)
            sustain_editor_holds(tick, birth, time, clock);
    }
    if (!playing) {
        pending.clear();
        holds.clear();
    } else
        std::erase_if(holds,
                      [time](const Hold& h) { return time >= h.chartEnd; });
    expire(clock);
    wasPlaying = playing;
    drawClock = clock;
    lastChartTime = time;
    lastTick = now;
}

void DynamiteParticles::sample_chart(double time) {
    bool rebuild = !chart;
    if (!chart) {
        reset();
        chart.emplace();
    }
    const auto now =
        static_cast<int64_t>(std::floor(time * TICKS_PER_MS + TICK_EPSILON));
    rebuild |= chart->refresh() || time < drawClock || now - lastTick > 24;
    if (!rebuild && time == drawClock)
        return;
    if (rebuild)
        for (auto& lane : lanes)
            lane.clear();
    const auto first = rebuild ? now - 24 : lastTick + 1;
    int64_t emissionTick = first - 1;
    if (first <= now)
        chart->visit(first, now,
                     [&](int64_t tick, int side, float center, double width,
                         bool sustained, uint64_t seed) {
                         const double birth = tick / TICKS_PER_MS;
                         if (emissionTick != tick) {
                             expire(birth);
                             emissionTick = tick;
                         }
                         if (!has_room(side, birth))
                             return;
                         randomState = seed;
                         const float w = emission_width(side, width);
                         const Emitter e{side, center, .45f * w};
                         if (sustained)
                             sustain(e, birth);
                         else
                             burst({e, tick, burst_amount(w), false}, birth);
                     });
    expire(time);
    drawClock = time;
    lastTick = now;
}

size_t DynamiteParticles::count() const {
    return lanes[0].size() + lanes[1].size() + lanes[2].size();
}

size_t DynamiteParticles::capacity_bytes() const {
    return count() * sizeof(Particle) + pending.size() * sizeof(Burst) +
           admitted.size() * sizeof(Admission) +
           holds.capacity() * sizeof(Hold) +
           (chart ? chart->capacity_bytes() : 0);
}

size_t DynamiteParticles::render(char* out) const {
    if (!count())
        return 0;
    const auto& atlas = get_sprite_manager().get_sprite("sprDynamiteHitAtlas");
    const auto uv0 = atlas.pos_to_uv({454, 818}),
               uv1 = atlas.pos_to_uv({518, 882});
    const auto start = out;
    // A lane is in birth order. Fragments emitted on the same 60 Hz step
    // share their age and rotation, but keep their individual size and life.
    double birth = std::numeric_limits<double>::quiet_NaN();
    float age = 0, seconds = 0;
    glm::vec2 rotation{};
    for (const auto& lane : lanes)
        for (const auto& p : lane) {
            if (p.birth != birth) {
                birth = p.birth;
                age = static_cast<float>(std::max(0.0, drawClock - birth));
                seconds = age * .001f;
                const float angle =
                    -240.f * (std::numbers::pi_v<float> / 180) * seconds;
                rotation = {std::cos(angle), std::sin(angle)};
            }
            const float u = age / p.lifetime;
            const float r = p.size * .5f * (1 - .4f * u * u * (1 + u));
            constexpr float key = 53585.f / 65535;
            const auto alpha =
                std::lround(u < key ? std::lerp(255.f, 190.f, u / key)
                                    : 190.f * (1 - u) / (1 - key));
            const glm::vec2 dx = r * rotation, dy(-dx.y, dx.x);
            const auto c = p.origin + p.velocity * seconds;
            vertex_quad_write(out, c - dx - dy, c + dx - dy, c - dx + dy,
                              c + dx + dy, uv0, {uv1.x, uv0.y}, {uv0.x, uv1.y},
                              uv1, glm::u8vec4(255, 255, 255, alpha));
        }
    return static_cast<size_t>(out - start);
}
