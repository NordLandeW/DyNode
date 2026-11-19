#include "dynamiteFx.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

#include "activation.h"
#include "notePoolManager.h"
#include "render.h"
#include "vertex.h"

namespace {
constexpr double FRAME_MS = 400, LIGHT_MS = 550;
constexpr int LIGHT = 20, HOLDING = 21;
// Row 384 is the last lit scanline of the 400-texel HEPLight frame.
constexpr int LIGHT_ROWS = 385, LIGHT_CELL = 400;
constexpr size_t FRAME_BUDGET = 500, SHADOW_BUDGET = 500;
constexpr size_t QUAD_BYTES = VERTEX_BYTES * 6;
constexpr double noHit = std::numeric_limits<double>::infinity();

glm::vec4 region(int index) {
    // All cells have gutters; return top-left and dimensions in atlas texels.
    if (index < 18)
        return {2 + 204 * (index % 5), 2 + 204 * (index / 5), 200, 200};
    if (index < 20)
        return {2 + 204 * (index - 18), 818, 200, 200};
    if (index == LIGHT)
        return {614, 614, LIGHT_CELL, LIGHT_ROWS};
    int k = index - HOLDING;
    return {410, 818 + k * 9, k < 2 ? 9 : 22, 5};
}

glm::vec2 origin(const DynamiteFx::Hit& h) {
    float c = static_cast<float>(pos_to_horzPos(h.position, h.side));
    if (h.side == 0)
        return {c, BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM};
    return {h.side == 1 ? JUDGE_LINE_SIDE_FROM_EDGE
                        : BASE_RES_W - JUDGE_LINE_SIDE_FROM_EDGE,
            c};
}

int light_alpha(double age) {
    constexpr double t[] = {0, 15420, 39514, 65535};
    constexpr double a[] = {255, 171, 48, 0};
    const double u = age / LIGHT_MS * 65535;
    const int i = u < t[1] ? 0 : u < t[2] ? 1 : 2;
    return static_cast<int>(
        std::lround(127.0 / 255 *
                    std::lerp(a[i], a[i + 1], (u - t[i]) / (t[i + 1] - t[i]))));
}
}  // namespace

void DynamiteFx::reset() {
    editorHits.clear();
    for (auto& p : passes)
        p.clear();
    mixerAge.fill(noHit);
}

void DynamiteFx::trigger_hit(Hit hit, double noteTime, bool light) {
    if (hit.side >= 0 && hit.side < 3 && (!hit.mixer || light))
        editorHits.push_back({hit, noteTime, !hit.mixer, light});
}

size_t DynamiteFx::capacity_bytes() const {
    return sizeof(EditorHit) * editorHits.capacity() +
           sizeof(Quad) * (passes[0].capacity() + passes[1].capacity() +
                           passes[2].capacity());
}

void DynamiteFx::add_visible(int pass, Quad quad) {
    // Each effect touches its line; only the transverse span can miss the
    // viewport.
    const bool front = quad.side == 0;
    const float c = front ? quad.origin.x : quad.origin.y;
    const float r = std::abs(quad.size.x) * .5f;
    if (r > 0 && c + r > 0 && c - r < (front ? BASE_RES_W : BASE_RES_H))
        passes[pass].push_back(quad);
}

void DynamiteFx::hit_sample(Hit hit, double age, bool frame, bool light) {
    const auto pos = origin(hit);
    const double w = 5.6 * hit.width * (hit.side == 0 ? 1.0 : 0.48);
    if (frame && age < FRAME_MS) {
        const double u = age / FRAME_MS;
        const int index = static_cast<int>(25 * .9999 * u * (2 - u));
        if (index < LIGHT) {
            const float size = static_cast<float>(std::min<double>(
                BASE_RES_W, std::max(2.8, w) * DYNAMITE_WORLD_PX));
            add_visible(
                0, {hit.side, index, pos, {size, size}, 0, glm::u8vec4(255)});
        }
    }
    if (light && age < LIGHT_MS) {
        const int alpha = light_alpha(age);
        if (alpha) {
            const float width = static_cast<float>(std::min<double>(
                BASE_RES_W, 1.8 * std::max(.7, w) * DYNAMITE_WORLD_PX));
            const float height = static_cast<float>(10.08 * DYNAMITE_WORLD_PX *
                                                    LIGHT_ROWS / LIGHT_CELL);
            add_visible(2, {hit.side,
                            LIGHT,
                            pos,
                            {width, height},
                            height * .5f,
                            {255, 255, 255, alpha}});
        }
    }
}

void DynamiteFx::holding_sample(Hit hit, double age) {
    const int k = std::min(3, static_cast<int>(std::fmod(age * .06, 5.0)));
    const double sx =
        (hit.width * (hit.side == 0 ? 40.0 / 9 : 32.0 / 15) - 8.0 / 45) * .5;
    add_visible(1, {hit.side,
                    HOLDING + k,
                    origin(hit),
                    {(k < 2 ? 9 : 22) * sx * DYNAMITE_WORLD_PX / 7.5,
                     5 * DYNAMITE_WORLD_PX / 7.5},
                    0,
                    glm::u8vec4(255)});
}

size_t DynamiteFx::prepare(double time, double clock, int setting, int mixers,
                           bool playback, bool playing) {
    const bool showFrames = playback || playing;
    const bool full = setting == 1;
    if (!full && setting != 2) {
        reset();
        return 0;
    }
    mixerAge.fill(noHit);
    for (auto& p : passes)
        p.clear();
    auto& pool = get_note_pool_manager();
    auto mixer = [=](int side) {
        return side > 0 && (mixers & (1 << (side - 1)));
    };
    // Read heads and actual SUB endpoints that have crossed the line. A seek
    // goes straight to this window; no simulation or editor activation replay.
    auto frame_room = [&] {
        return showFrames && passes[0].size() + passes[1].size() < FRAME_BUDGET;
    };
    size_t mixerLights = 0;
    auto light_room = [&] {
        return passes[2].size() + mixerLights < SHADOW_BUDGET;
    };
    auto sample_mixer = [&](int side, double age) {
        // One visible tray per side; older hits on the same tray add no extra
        // light.
        if (age * .0048 < 1 && std::isinf(mixerAge[side - 1])) {
            mixerAge[side - 1] = age;
            ++mixerLights;
        }
    };
    if (playback) {
        editorHits.clear();
        if (pool.is_ooo())
            pool.array_sort_request();
        pool.read_recent_notes(time - LIGHT_MS, time, [&](const Note& n) {
            if (n.side < 0 || n.side > 2)
                return true;
            const double age = time - n.time;
            const bool light = full && light_room();
            if (mixer(n.side)) {
                if (light)
                    sample_mixer(n.side, age);
            } else {
                Hit hit{n.side, n.position, n.width, n.time};
                hit_sample(hit, age, frame_room(), light);
                if (frame_room() && n.get_note_type() == NOTE_TYPE::HOLD &&
                    age < n.lastTime)
                    holding_sample(hit, age);
            }
            return frame_room() || (full && light_room());
        });
    }
    // The sustained ribbon is a state of a live hold, not a judgment event.
    // Playback already emitted ribbons for heads inside the sampling window.
    const auto& active = get_note_activation_manager();
    const auto& holds =
        playback ? active.get_lasting_holds() : active.get_active_holds();
    const auto byStart = [](const auto& h, double t) { return h.first < t; };
    auto hold = playback ? std::lower_bound(holds.begin(), holds.end(),
                                            time - LIGHT_MS, byStart)
                         : std::upper_bound(holds.begin(), holds.end(), time,
                                            [](double t, const auto& h) {
                                                return t < h.first;
                                            });
    auto draw_holds_after = [&](double oldest) {
        while (frame_room() && hold != holds.begin() &&
               std::prev(hold)->first > oldest) {
            const auto& id = (--hold)->second;
            if (!pool.note_exists(id))
                continue;
            const auto& n = pool.get_note_unsafe(id);
            if (n.side < 0 || n.side > 2 || mixer(n.side) || n.time > time ||
                n.time + n.lastTime <= time)
                continue;
            holding_sample({n.side, n.position, n.width, n.time},
                           time - n.time);
        }
    };
    if (!playback) {
        for (auto& e : editorHits) {
            const double age = clock - e.hit.time;
            e.frame &= playing && age < FRAME_MS;
            e.light &= full && age < LIGHT_MS;
        }
        std::erase_if(editorHits, [&](const EditorHit& e) {
            return (!e.frame && !e.light) || clock < e.hit.time ||
                   mixer(e.hit.side) != e.hit.mixer;
        });
        for (auto it = editorHits.rbegin();
             it != editorHits.rend() &&
             (frame_room() || (full && light_room()));
             ++it) {
            const auto& h = it->hit;
            const double age = clock - h.time;
            if (it->frame)
                draw_holds_after(it->noteTime);
            const bool light = it->light && light_room();
            if (!h.mixer)
                hit_sample(h, age, it->frame && frame_room(), light);
            else if (light)
                sample_mixer(h.side, age);
        }
    }
    draw_holds_after(-noHit);
    return std::max({passes[0].size(), passes[1].size(), passes[2].size()}) *
           QUAD_BYTES;
}

size_t DynamiteFx::render(char* out, int pass) const {
    const auto& quads = passes.at(pass);
    if (quads.empty())
        return 0;
    const auto& sprite = get_sprite_manager().get_sprite("sprDynamiteHitAtlas");
    for (const auto& q : quads) {
        const auto rect = region(q.region);
        glm::vec2 uv0 = sprite.pos_to_uv({rect.x, rect.y});
        glm::vec2 uv1 = sprite.pos_to_uv({rect.x + rect.z, rect.y + rect.w});
        const glm::vec2 inward =
            q.side == 0 ? glm::vec2(0, -1) : glm::vec2(q.side == 1 ? 1 : -1, 0);
        const glm::vec2 dx = glm::vec2(-inward.y, inward.x) * (q.size.x * .5f);
        const glm::vec2 dy = inward * (q.size.y * .5f);
        // Side hit billboards share a rotation. The holding sprite instead
        // inherits the lane transform: its positive X points up on either side.
        if (q.region < LIGHT && q.side == 2)
            std::swap(uv0, uv1);
        else if (q.region >= HOLDING && q.side == 1)
            std::swap(uv0.x, uv1.x);
        const auto center = q.origin + inward * static_cast<float>(q.travel);
        vertex_quad_write(out, center - dx + dy, center + dx + dy,
                          center - dx - dy, center + dx - dy, uv0,
                          {uv1.x, uv0.y}, {uv0.x, uv1.y}, uv1, q.color);
    }
    return quads.size() * QUAD_BYTES;
}
