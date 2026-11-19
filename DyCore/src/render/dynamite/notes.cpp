#include "notes.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <taskflow/taskflow.hpp>

#include "activation.h"
#include "capacity.h"
#include "notePoolManager.h"
#include "profile.h"
#include "render.h"
#include "vertex.h"

namespace {
constexpr size_t QUAD_BYTES = 6 * VERTEX_BYTES;
enum Part : unsigned { BODY = 1, UP = 2, DOWN = 4 };
// Positive mapping IDs select the four corresponding shader slice descriptors.
enum Slice { TAP = 1, SLIDE, CAP_UP, CAP_DOWN };

double far_edge(int side) {
    return side == 0 ? BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM
                     : BASE_RES_W - JUDGE_LINE_SIDE_FROM_EDGE;
}

struct Sprites {
    const SpriteData& tap;
    const SpriteData& slide;
    const SpriteData& inner;
    const SpriteData& lit;
    const SpriteData& rail;
    const SpriteData& up;
    const SpriteData& down;
};

// Across follows sprite X, travel increases from the judgement line towards
// future notes. The same writer handles all three lanes without trig or pivots.
struct Lane {
    int side;
    float center;
    glm::u8vec4 color;

    glm::vec2 point(double offset, double travel) const {
        if (side == 0)
            return {center + offset,
                    BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM - travel};
        if (side == 1)
            return {JUDGE_LINE_SIDE_FROM_EDGE + travel, center + offset};
        return {BASE_RES_W - JUDGE_LINE_SIDE_FROM_EDGE - travel,
                center - offset};
    }

    void quad(char*& out, double left, double right, double farTravel,
              double nearTravel, glm::vec2 uv0, glm::vec2 uv1,
              glm::vec2 mapping = {}) const {
        vertex_quad_write(out, point(left, farTravel), point(right, farTravel),
                          point(left, nearTravel), point(right, nearTravel),
                          uv0, {uv1.x, uv0.y}, {uv0.x, uv1.y}, uv1, color,
                          mapping);
    }

    void strip(char*& out, const SpriteData& sprite, Slice slice, double left,
               double right, double distance) const {
        const double halfHeight = sprite.size.y * DYNAMITE_TEXEL * 0.5;
        const float texels =
            static_cast<float>(std::abs(right - left) / DYNAMITE_TEXEL);
        quad(out, left, right, distance + halfHeight, distance - halfHeight,
             {0, sprite.uv0.y}, {texels, sprite.uv1.y},
             {((right < left) != (side == 2)) ? -texels : texels,
              static_cast<float>(slice)});
    }
};

}  // namespace

size_t DynamiteBatch::vertex_buffer_bound(size_t notes, size_t holds) {
    constexpr size_t maxQuads = std::numeric_limits<size_t>::max() / QUAD_BYTES;
    if (holds > notes || holds > maxQuads / 5 ||
        notes - holds > (maxQuads - 5 * holds) / 2)
        throw std::length_error("Too many playback notes");
    return (5 * holds + 2 * (notes - holds)) * QUAD_BYTES;
}

bool DynamiteBatch::reserve(size_t notes) {
    return reserve_with_headroom(items, notes);
}

size_t DynamiteBatch::capacity_bytes() const {
    return items.capacity() * sizeof(Item);
}

size_t DynamiteBatch::render(char* buffer, const PlaybackFrame& frame,
                             tf::Executor& executor, tf::Taskflow& tasks,
                             size_t workerCount, size_t& taskSubmissions) {
    PROFILE_STATIC_SCOPE("Render Dynamite Notes");
    items.clear();
    fillVertices = 0;
    const auto& active = get_note_activation_manager().get_active_notes();
    if (active.empty())
        return 0;
    const auto& atlas = get_sprite_manager();
    const Sprites sprites{atlas.get_sprite("sprDynamiteTap"),
                          atlas.get_sprite("sprDynamiteSlideHalf"),
                          atlas.get_sprite("sprDynamiteHoldInner"),
                          atlas.get_sprite("sprDynamiteHoldLit"),
                          atlas.get_sprite("sprDynamiteHoldSide"),
                          atlas.get_sprite("sprDynamiteHoldUp"),
                          atlas.get_sprite("sprDynamiteHoldDown")};
    size_t bodies = 0, edges = 0, slides = 0, taps = 0;
    auto& pool = get_note_pool_manager();
    const double margin = sprites.up.size.y * DYNAMITE_TEXEL * 0.5;
    const double minimum[] = {dynamite_min_duration(frame.frontSpeed, 0),
                              dynamite_min_duration(frame.sideSpeed, 1)};
    for (const auto& entry : active) {
        const Note& note = pool.get_note_unsafe(entry.id, entry.index);
        const double speed = frame.speed(note.side);
        const double travel = (note.time - frame.time) * speed;
        if (note.get_note_type() == NOTE_TYPE::HOLD) {
            const double end =
                note.time + std::max(note.lastTime, minimum[note.side != 0]);
            if (end <= frame.time)
                continue;
            const double head = std::max(0.0, travel);
            const double tail = (end - frame.time) * speed;
            const double farEdge = far_edge(note.side);
            unsigned parts = 0;
            if (std::min(tail, farEdge) > head)
                parts |= BODY;
            if (tail <= farEdge + margin)
                parts |= UP;
            if (head <= farEdge + margin)
                parts |= DOWN;
            if (!parts)
                continue;
            items.push_back({&note, bodies, edges, head, tail, parts});
            bodies += (parts & BODY) != 0;
            edges += (parts & BODY ? 2 : 0) + ((parts & UP) != 0) +
                     ((parts & DOWN) != 0);
        } else if (note.time > frame.time) {
            if (note.get_note_type() == NOTE_TYPE::CHAIN) {
                items.push_back({&note, slides, 0, travel, 0, 0});
                slides += 2;
            } else if (note.get_note_type() == NOTE_TYPE::NORMAL) {
                items.push_back({&note, taps++, 0, travel, 0, 0});
            }
        }
    }

    const size_t total = (bodies + edges + slides + taps) * QUAD_BYTES;
    if (total == 0)
        return 0;
    auto draw = [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            const auto& item = items[i];
            const auto& note = *item.note;
            const bool front = note.side == 0;
            const double chartSpan = note.width * (front ? 300.0 : 150.0);
            const auto pos =
                get_note_pos(note, frame.time, frame.speed(note.side));
            const Lane lane{
                note.side,
                front ? pos.x : pos.y,
                {255, 255, 255,
                 static_cast<uint8_t>(get_note_alpha(note.side, pos) * 255)}};
            if (note.get_note_type() == NOTE_TYPE::HOLD) {
                const double width = std::max(60.0, chartSpan) -
                                     4.0 * DYNAMITE_WORLD_PX +
                                     sprites.inner.size.x * DYNAMITE_TEXEL;
                char* edge = buffer + (bodies + item.edgeOffset) * QUAD_BYTES;
                if (item.parts & BODY) {
                    const double farTravel =
                        std::min(item.tail, far_edge(note.side));
                    const auto& sprite =
                        note.time <= frame.time ? sprites.lit : sprites.inner;
                    const double period = sprite.size.y * 2 * DYNAMITE_TEXEL;
                    const double phase =
                        std::fmod(item.tail - farTravel, period) / period;
                    // Negative span encodes a vertically flipped repeat. The
                    // left lane owns the opposite edge of exact joins under the
                    // top-left fill rule.
                    const glm::vec2 mapping{
                        note.side == 1 ? -1.0f - sprite.uv0.y : sprite.uv0.y,
                        -sprite.uvSize.y};
                    char* body = buffer + item.offset * QUAD_BYTES;
                    // Fill under the opaque core of each rail; keep rails and
                    // caps fixed.
                    const double fillHalf = width * .5 + 3 * DYNAMITE_TEXEL;
                    lane.quad(body, -fillHalf, fillHalf, farTravel, item.head,
                              {sprite.uv0.x, static_cast<float>(phase)},
                              {sprite.uv1.x,
                               static_cast<float>(
                                   phase + (farTravel - item.head) / period)},
                              mapping);
                    const double railWidth =
                        sprites.rail.size.x * DYNAMITE_TEXEL;
                    const double offset =
                        width / 2 + 0.02 * DYNAMITE_WORLD_PX + railWidth / 2;
                    lane.quad(edge, -offset, -offset + railWidth, farTravel,
                              item.head, sprites.rail.uv0, sprites.rail.uv1);
                    lane.quad(edge, offset, offset - railWidth, farTravel,
                              item.head, sprites.rail.uv0, sprites.rail.uv1);
                }
                if (item.parts & UP) {
                    const double half =
                        width * 0.5 +
                        sprites.up.drawSetting.data[0] * DYNAMITE_TEXEL;
                    lane.strip(edge, sprites.up, CAP_UP, -half, half,
                               item.tail);
                }
                if (item.parts & DOWN) {
                    const double half =
                        width * 0.5 +
                        sprites.down.drawSetting.data[0] * DYNAMITE_TEXEL;
                    lane.strip(edge, sprites.down, CAP_DOWN, -half, half,
                               item.head);
                }
            } else {
                const double width =
                    std::max(DYNAMITE_WORLD_PX,
                             chartSpan - (8.0 / 45) * DYNAMITE_WORLD_PX);
                char* out =
                    buffer +
                    (bodies + edges + item.offset +
                     (note.get_note_type() == NOTE_TYPE::NORMAL ? slides : 0)) *
                        QUAD_BYTES;
                if (note.get_note_type() == NOTE_TYPE::CHAIN) {
                    const double half = width / 2 + 0.14 * DYNAMITE_WORLD_PX;
                    lane.strip(out, sprites.slide, SLIDE, -half, 0, item.head);
                    lane.strip(out, sprites.slide, SLIDE, half, 0, item.head);
                } else {
                    lane.strip(out, sprites.tap, TAP, -width / 2, width / 2,
                               item.head);
                }
            }
        }
    };
    if (workerCount > 1 && total >= MULTITHREAD_RENDERING_BYTE_THRESHOLD) {
        tasks.clear();
        const size_t chunks = std::min(items.size(), workerCount * 4);
        const size_t chunkSize = (items.size() + chunks - 1) / chunks;
        for (size_t begin = 0; begin < items.size(); begin += chunkSize) {
            const size_t end = std::min(begin + chunkSize, items.size());
            tasks.emplace([&, begin, end] { draw(begin, end); });
        }
        ++taskSubmissions;
        executor.run(tasks).get();
    } else {
        draw(0, items.size());
    }
    fillVertices = bodies * 6;
    return total;
}
