#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <limits>
#include <vector>

#include "activation.h"
#include "capacity.h"
#include "notePoolManager.h"
#include "render.h"
#include "vertex.h"

namespace {
void install_render_sprites() {
    auto add = [](const char* name, SPRITE_DRAW_TYPE type,
                  std::array<int, 4> cuts) {
        SpriteData sprite{};
        sprite.name = name;
        sprite.size = {120.0f, 100.0f};
        sprite.uv0 = {0.0f, 0.0f};
        sprite.uv1 = {1.0f, 1.0f};
        sprite.drawSetting.type = type;
        std::copy(cuts.begin(), cuts.end(), sprite.drawSetting.data);
        sprite.caculate_uv_values();
        get_sprite_manager().add_sprite(sprite);
    };
    add("sprNote", SPRITE_DRAW_TYPE::SEG_3, {20, 20});
    add("sprChain", SPRITE_DRAW_TYPE::SEG_5, {20, 80, 20});
    add("sprHoldEdge", SPRITE_DRAW_TYPE::SLICE_9, {20, 20, 20, 20});
    add("sprHold", SPRITE_DRAW_TYPE::REPEAT_VERT, {});
    add("sprHoldGrey", SPRITE_DRAW_TYPE::NORMAL, {});
}

struct RenderCleanup {
    ~RenderCleanup() {
        shutdown_note_rendering();
        get_note_pool_manager().clear_notes();
    }
};

struct RenderVertex {
    glm::vec2 position;
    glm::vec2 uv;
    glm::u8vec4 color;
    glm::vec2 mapping;
};
static_assert(sizeof(RenderVertex) == 28);
static_assert(sizeof(RenderVertex) == VERTEX_BYTES);

std::vector<RenderVertex> render_vertices(double time, int state) {
    auto& activation = get_note_activation_manager();
    activation.set_range(time, 1000.0);
    activation.recalculate();
    const size_t bound = prepare_note_rendering();
    std::vector<char> buffer(bound + 64, static_cast<char>(0xA5));
    const size_t bytes =
        render_active_notes(buffer.data(), time, 1000.0, state);
    REQUIRE(bytes <= bound);
    REQUIRE(bytes % sizeof(RenderVertex) == 0);
    CHECK(std::all_of(buffer.begin() + bytes, buffer.end(), [](char value) {
        return value == static_cast<char>(0xA5);
    }));
    std::vector<RenderVertex> result(bytes / sizeof(RenderVertex));
    if (bytes != 0)
        std::memcpy(result.data(), buffer.data(), bytes);
    return result;
}

void install_dynamite_sprites() {
    auto add = [](const char* name, glm::vec2 size, float top, int left = 0,
                  int right = 0) {
        SpriteData sprite{};
        sprite.name = name;
        sprite.size = size;
        sprite.uv0 = {0.125f, top};
        sprite.uv1 = sprite.uv0 + size / 2048.0f;
        sprite.drawSetting.data[0] = left;
        sprite.drawSetting.data[1] = right;
        sprite.caculate_uv_values();
        get_sprite_manager().add_sprite(sprite);
    };
    add("sprDynamiteTap", {288, 36}, 0.0f, 30, 30);
    add("sprDynamiteSlideHalf", {155, 84}, 0.125f, 30, 57);
    add("sprDynamiteHoldInner", {254, 125}, 0.25f);
    add("sprDynamiteHoldLit", {254, 125}, 0.375f);
    add("sprDynamiteHoldSide", {24, 46}, 0.5f);
    add("sprDynamiteHoldUp", {288, 43}, 0.625f, 19, 19);
    add("sprDynamiteHoldDown", {292, 43}, 0.75f, 21, 21);
}

void add_playback_note(const std::string& id, int type, int side, double time,
                       double width, double duration = 0) {
    Note note{};
    note.noteID = id;
    note.type = type;
    note.side = side;
    note.position = 2.5;
    note.time = time;
    note.width = width;
    note.lastTime = duration;
    note.subNoteID = id + "s";
    REQUIRE(create_note(note, false) == 0);
}

std::vector<RenderVertex> playback_vertices(PlaybackFrame frame, int pass = 2) {
    const size_t bound = prepare_playback_notes(frame);
    const auto prepared = get_note_rendering_stats();
    constexpr char canary = '\x5a';
    std::vector<char> buffer(bound + 64, canary);
    const size_t bytes = render_playback_notes(buffer.data(), pass);
    REQUIRE(bytes <= bound);
    REQUIRE(bytes % sizeof(RenderVertex) == 0);
    CHECK(std::all_of(buffer.begin() + bytes, buffer.end(),
                      [](char c) { return c == canary; }));
    CHECK(get_note_rendering_stats().capacityGrowths ==
          prepared.capacityGrowths);
    std::vector<RenderVertex> vertices(bytes / sizeof(RenderVertex));
    if (bytes)
        std::memcpy(vertices.data(), buffer.data(), bytes);
    return vertices;
}

}  // namespace

TEST_CASE("CapacityGrowthAddsHeadroomWithoutOverflow") {
    std::vector<int> values{1, 2, 3};
    REQUIRE(reserve_with_headroom(values, 200));
    const auto capacity = values.capacity();
    CHECK(capacity > 200);
    CHECK(values == std::vector<int>{1, 2, 3});
    CHECK_FALSE(reserve_with_headroom(values, 201));
    CHECK(values.capacity() == capacity);
    const auto maximum = (std::numeric_limits<size_t>::max)();
    CHECK(capacity_with_headroom(0, maximum - 4, maximum) == maximum);
    CHECK_THROWS_AS(capacity_with_headroom(0, 101, 100), std::length_error);
}

TEST_CASE("RenderFramePreparationCoversEveryPassAndSurvivesDensityChanges") {
    RenderCleanup cleanup;
    auto& notes = get_note_pool_manager();
    notes.clear_notes();
    install_render_sprites();
    initialize_note_rendering();
    auto add_notes = [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            Note note{};
            note.noteID = std::format("{:09}", i);
            note.side = static_cast<int>(i % 3);
            note.type = static_cast<int>(i % 3);
            note.time = 100.0 + static_cast<double>(i % 10) * 0.01;
            note.lastTime = note.get_note_type() == NOTE_TYPE::HOLD ? 3.0 : 0.0;
            note.width = 1.0;
            note.position = 2.0;
            REQUIRE(notes.create_note(note));
        }
    };
    add_notes(0, 12000);
    auto render = [&](double time) {
        auto& activation = get_note_activation_manager();
        activation.set_range(time, 1000.0);
        activation.recalculate();
        const size_t bound = prepare_note_rendering();
        const auto prepared = get_note_rendering_stats();
        std::vector<char> buffer(bound + 4096, static_cast<char>(0xA5));
        std::array<std::vector<char>, 3> output;
        for (int state : {1, 0, 2}) {
            const auto size =
                render_active_notes(buffer.data(), time, 1000.0, state);
            REQUIRE(size <= bound);
            CHECK(std::all_of(
                buffer.begin() + bound, buffer.end(),
                [](char value) { return value == static_cast<char>(0xA5); }));
            CHECK(get_note_rendering_stats().capacityGrowths ==
                  prepared.capacityGrowths);
            CHECK(get_note_rendering_stats().workspaceCapacityBytes ==
                  prepared.workspaceCapacityBytes);
            output[state].assign(buffer.begin(), buffer.begin() + size);
        }
        return output;
    };
    render(90.0);
    const auto dense = render(100.0);
    REQUIRE_FALSE(dense[2].empty());
    const auto capacity = get_note_rendering_stats().workspaceCapacityBytes;
    CHECK(render(100.0) == dense);
    render(102.0);
    CHECK(render(100.0) == dense);
    CHECK(get_note_rendering_stats().workspaceCapacityBytes == capacity);
    add_notes(12000, 24000);
    const auto expanded = render(100.0);
    CHECK(expanded[2].size() > dense[2].size());
    CHECK(get_note_rendering_stats().workspaceCapacityBytes > capacity);
    CHECK(render(100.0) == expanded);
    CHECK_NOTHROW(shutdown_note_rendering());
    CHECK_NOTHROW(shutdown_note_rendering());
    CHECK(get_note_rendering_stats().workspaceCapacityBytes == 0);
}

TEST_CASE("SegmentedNotesUseSingleQuadsWithLocalCoordinatesOnEverySide") {
    RenderCleanup cleanup;
    auto& notes = get_note_pool_manager();
    install_render_sprites();
    initialize_note_rendering();
    const std::array<glm::vec2, 6> expectedUvs = {
        glm::vec2{0, 0}, {1, 0}, {0, 1}, {1, 0}, {0, 1}, {1, 1}};
    for (int type : {0, 1}) {
        for (int side : {0, 1, 2}) {
            for (double width : {0.25, 1.0, 3.125}) {
                CAPTURE(type);
                CAPTURE(side);
                CAPTURE(width);
                notes.clear_notes();
                Note note{};
                note.noteID = "single";
                note.type = type;
                note.side = side;
                note.time = 100.25;
                note.width = width;
                note.position = 2.5;
                REQUIRE(notes.create_note(note));
                const auto vertices = render_vertices(100.0, 2);
                REQUIRE(vertices.size() == 6);
                const float pixelWidth = static_cast<float>(std::max(
                    120.0, width * (side == 0 ? 300.0 : 150.0) - 30.0));
                const glm::vec2 center =
                    side == 0 ? glm::vec2{960, 693}
                              : glm::vec2{side == 1 ? 362 : 1558, 540};
                const glm::vec2 halfSize =
                    side == 0 ? glm::vec2{pixelWidth * 0.5f, 50}
                              : glm::vec2{50, pixelWidth * 0.5f};
                glm::vec2 minimum = vertices[0].position;
                glm::vec2 maximum = minimum;
                for (size_t i = 0; i < vertices.size(); ++i) {
                    const auto& vertex = vertices[i];
                    CHECK(vertex.uv ==
                          expectedUvs[i] * glm::vec2(pixelWidth, 1));
                    CHECK(vertex.mapping.x == doctest::Approx(pixelWidth));
                    CHECK(vertex.mapping.y == type + 1);
                    CHECK(vertex.color == glm::u8vec4(255));
                    minimum = glm::min(minimum, vertex.position);
                    maximum = glm::max(maximum, vertex.position);
                }
                for (int axis = 0; axis < 2; ++axis) {
                    CHECK(minimum[axis] ==
                          doctest::Approx(center[axis] - halfSize[axis])
                              .epsilon(0.00001));
                    CHECK(maximum[axis] ==
                          doctest::Approx(center[axis] + halfSize[axis])
                              .epsilon(0.00001));
                }
            }
        }
    }
}

TEST_CASE("HoldRepeatsUseOneQuadAndFourQuadHollowFrame") {
    RenderCleanup cleanup;
    auto& notes = get_note_pool_manager();
    install_render_sprites();
    initialize_note_rendering();
    const std::array<glm::vec2, 6> corners = {glm::vec2{0, 0}, {1, 0}, {0, 1},
                                              {1, 0},          {0, 1}, {1, 1}};
    for (int side : {0, 1, 2}) {
        for (double width : {0.25, 1.0, 3.125}) {
            for (double length : {100.0, 125.0, 375.0, 900.0}) {
                CAPTURE(side);
                CAPTURE(width);
                CAPTURE(length);
                notes.clear_notes();
                Note note{};
                note.noteID = "hold";
                note.type = 2;
                note.side = side;
                note.time = 100.25;
                note.lastTime = length / 1000.0;
                note.width = width;
                note.position = 2.5;
                REQUIRE(notes.create_note(note));
                const auto bar = render_vertices(100.0, 1);
                REQUIRE(bar.size() == 6);
                for (const auto& vertex : bar) {
                    CHECK(vertex.mapping == glm::vec2(side == 1 ? 1 : 0, 4));
                    CHECK(vertex.uv.x >= 0.0f);
                    CHECK(vertex.uv.x <= 1.0f);
                }
                CHECK(bar[0].uv.y == 0.0f);
                CHECK(bar[5].uv.y == doctest::Approx(length / 100.0));
                CHECK(glm::length(bar[2].position - bar[0].position) ==
                      doctest::Approx(length));
                const auto edge = render_vertices(100.0, 2);
                REQUIRE(edge.size() == 24);
                const float w = static_cast<float>(std::max(
                    120.0, width * (side == 0 ? 300.0 : 150.0) - 30.0));
                const float h = static_cast<float>(length);
                // These four non-overlapping strips must leave the center open.
                const std::array<glm::vec4, 4> bounds = {
                    glm::vec4{0, 0, w, 20},
                    {0, 20, 20, h - 20},
                    {w - 20, 20, w, h - 20},
                    {0, h - 20, w, h}};
                const std::array<glm::vec4, 4> uvs = {glm::vec4{0, 0, w, 0.2f},
                                                      {0, 0.2f, 1.0f / 6, 0.8f},
                                                      {5.0f / 6, 0.2f, 1, 0.8f},
                                                      {0, 0.8f, w, 1}};
                const auto xAxis =
                    glm::normalize(bar[1].position - bar[0].position);
                const auto yAxis =
                    glm::normalize(bar[2].position - bar[0].position);
                for (size_t i = 0; i < edge.size(); ++i) {
                    const size_t strip = i / 6;
                    const auto& corner = corners[i % 6];
                    const auto expectedPosition = glm::mix(
                        glm::vec2(bounds[strip]),
                        glm::vec2(bounds[strip].z, bounds[strip].w), corner);
                    const auto expectedUv =
                        glm::mix(glm::vec2(uvs[strip]),
                                 glm::vec2(uvs[strip].z, uvs[strip].w), corner);
                    const auto offset = edge[i].position - bar[0].position;
                    const glm::vec2 local{glm::dot(offset, xAxis),
                                          glm::dot(offset, yAxis)};
                    for (int axis = 0; axis < 2; ++axis) {
                        CHECK(local[axis] ==
                              doctest::Approx(expectedPosition[axis])
                                  .epsilon(0.0001));
                        CHECK(edge[i].uv[axis] ==
                              doctest::Approx(expectedUv[axis]));
                    }
                    CHECK(edge[i].mapping == (strip == 0 || strip == 3
                                                  ? glm::vec2(w, 3)
                                                  : glm::vec2(0)));
                    CHECK(edge[i].color == glm::u8vec4(255));
                }
            }
        }
    }
}

TEST_CASE("DynamitePlaybackOrdersAndOrientsEveryLayerAcrossLanes") {
    RenderCleanup cleanup;
    initialize_note_rendering();
    install_dynamite_sprites();
    auto& pool = get_note_pool_manager();
    const PlaybackFrame frame{100, 1.6, 1.44, true};
    for (int side : {0, 1, 2}) {
        CAPTURE(side);
        pool.clear_notes();
        add_playback_note("first", 2, side, 200, 1.0, 40);
        add_playback_note("second", 2, side, 201, 1.0, 40);
        add_playback_note("tap", 0, side, 200, 1.0);
        add_playback_note("slide", 1, side, 200, 1.0);
        const auto v = playback_vertices(frame);
        REQUIRE(v.size() == 13 * 6);
        const auto strip_width = [&](size_t q) {
            return glm::distance(v[6 * q].position, v[6 * q + 1].position);
        };
        CHECK(v[0].mapping.y < 0);
        CHECK(v[6].mapping.y < 0);
        for (int q : {2, 6}) {
            CHECK(v[q * 6].uv == glm::vec2(0.125f, 0.5f));
            CHECK(v[(q + 1) * 6].uv == v[q * 6].uv);
            CHECK(glm::dot(v[q * 6 + 1].position - v[q * 6].position,
                           v[(q + 1) * 6 + 1].position -
                               v[(q + 1) * 6].position) < 0);
            CHECK(v[(q + 2) * 6].uv.y == 0.625f);
            CHECK(v[(q + 3) * 6].uv.y == 0.75f);
        }
        CHECK(v[60].uv.y == 0.125f);
        CHECK(v[66].uv.y == 0.125f);
        CHECK(v[72].uv.y == 0.0f);
        CHECK(v[61].position == v[67].position);
        CHECK(v[60].mapping.x == -v[66].mapping.x);
        CHECK(strip_width(0) == doctest::Approx(side == 0 ? 258.6 : 102.6));
        CHECK(strip_width(2) == doctest::Approx(21.6));
        CHECK(strip_width(4) == doctest::Approx(side == 0 ? 292.8 : 136.8));
        CHECK(strip_width(5) == doctest::Approx(side == 0 ? 296.4 : 140.4));
        CHECK(strip_width(10) + strip_width(11) ==
              doctest::Approx(side == 0 ? 306.9 : 150.9));
        CHECK(strip_width(12) == doctest::Approx(side == 0 ? 288.0 : 132.0));
        const auto center = (v[72].position + v[77].position) * 0.5f;
        const glm::vec2 expected = side == 0
                                       ? glm::vec2(960, 783)
                                       : glm::vec2(side == 1 ? 256 : 1664, 540);
        CHECK(glm::distance(center, expected) <= 0.001f);
        CHECK(v[0].color == glm::u8vec4(255));
        CHECK(playback_vertices(frame, 0).empty());
        CHECK(playback_vertices(frame, 1).empty());
    }
}

TEST_CASE("DynamiteHoldVisualTailSurvivesSeeksWithoutExtendingParticles") {
    RenderCleanup cleanup;
    initialize_note_rendering();
    install_dynamite_sprites();
    auto& pool = get_note_pool_manager();
    for (int side : {0, 1, 2}) {
        CAPTURE(side);
        pool.clear_notes();
        add_playback_note("short", 2, side, 100, 1, 2);
        for (double time : {106.0, 101.0, 102.0, 115.0, 116.0, 99.0}) {
            CAPTURE(time);
            const auto v = playback_vertices({time, 1.6, 1.44, true});
            const auto& lasting =
                get_note_activation_manager().get_lasting_holds();
            CHECK(lasting.size() == (time > 100 && time < 102 ? 1 : 0));
            if (time >= 116) {
                CHECK(v.empty());
                continue;
            }
            REQUIRE(v.size() == 30);
            const float uvTop = time < 100 ? 0.25f : 0.375f;
            CHECK(v[0].mapping.x == (side == 1 ? -1.0f - uvTop : uvTop));
            const double pixels =
                (116.0 - std::max(100.0, time)) * (side == 0 ? 1.6 : 1.44);
            CHECK(glm::distance(v[0].position, v[2].position) ==
                  doctest::Approx(pixels).epsilon(0.0001));
            CHECK(v[5].uv.y == doctest::Approx(pixels / 225.0));
        }
        pool.clear_notes();
        add_playback_note("long", 2, side, 0, 1, 20000);
        const auto clipped = playback_vertices({100, 1.6, 1.44, true});
        REQUIRE(clipped.size() == 4 * 6);
        const double visible = side == 0 ? 943.0 : 1808.0;
        CHECK(glm::distance(clipped[0].position, clipped[2].position) ==
              doctest::Approx(visible));
        CHECK(clipped[0].uv.y ==
              doctest::Approx(
                  std::fmod(19900 * (side == 0 ? 1.6 : 1.44) - visible, 225.0) /
                  225.0));
        CHECK(clipped[5].uv.y - clipped[0].uv.y ==
              doctest::Approx(visible / 225.0));
        CHECK(clipped[18].uv.y ==
              0.75f);  // Remote tail cap culled; clamped head still drawn.
    }
}

TEST_CASE("PlaybackActivationAndProjectionShareTransitionSpeedAcrossStyles") {
    RenderCleanup cleanup;
    initialize_note_rendering();
    install_render_sprites();
    install_dynamite_sprites();
    auto& pool = get_note_pool_manager();
    pool.clear_notes();
    add_playback_note("left", 0, 1, 640, 1);
    add_playback_note("right", 0, 2, 640, 1);
    add_playback_note("pastfront", 0, 0, 740, 1);
    for (bool dynamite : {true, false}) {
        const auto v = playback_vertices({100, 1.6, 1.52, dynamite});
        REQUIRE(v.size() == 12);
        for (int i : {0, 1}) {
            const auto center =
                (v[6 * i].position + v[6 * i + 5].position) * 0.5f;
            CHECK(center.x == doctest::Approx(i == 0 ? 932.8 : 987.2));
            CHECK(center.y == doctest::Approx(540.0f));
            CHECK(v[6 * i].color.a == 72);
        }
        CHECK(playback_vertices({100, 1.6, 1.6, dynamite}).empty());
    }
    // At the settled Dynamix speed, all three passes are byte-for-byte the
    // legacy path.
    add_playback_note("hold", 2, 0, 80, 1, 240);
    add_playback_note("chain", 1, 1, 150, 1);
    for (int pass : {1, 0, 2}) {
        const auto v = playback_vertices({100, 1.6, 1.6, false}, pass);
        auto& a = get_note_activation_manager();
        a.set_range(100, 1.6);
        a.recalculate();
        std::vector<char> legacy(prepare_note_rendering());
        const size_t bytes = render_active_notes(legacy.data(), 100, 1.6, pass);
        REQUIRE(bytes == v.size() * sizeof(RenderVertex));
        REQUIRE(bytes > 0);
        CHECK(std::memcmp(legacy.data(), v.data(), bytes) == 0);
    }
}

TEST_CASE("DynamiteBatchBoundsStayIndependentOfChartWidthAndCoverDenseOutput") {
    RenderCleanup cleanup;
    initialize_note_rendering();
    install_dynamite_sprites();
    auto& pool = get_note_pool_manager();
    const PlaybackFrame frame{100, 1.6, 1.44, true};
    for (double width : {0.25, 1.0, 8192.0}) {
        pool.clear_notes();
        add_playback_note("slide", 1, 2, 150, width);
        const auto v = playback_vertices(frame);
        REQUIRE(v.size() == 12);
        if (width == 0.25) {
            CHECK(glm::distance(v[0].position, v[6].position) ==
                  doctest::Approx(86.4));
        }
        CHECK(v[1].position == v[7].position);
    }
    pool.clear_notes();
    add_playback_note("a", 2, 0, 150, 1, 200);
    const auto single = playback_vertices(frame);
    REQUIRE(single.size() == 30);
    constexpr size_t count = 3000;
    for (size_t i = 1; i < count; ++i) {
        add_playback_note(std::to_string(i), 2, 0, 150, 1, 200);
    }
    const auto dense = playback_vertices(frame);
    REQUIRE(dense.size() == count * 30);
    for (size_t i = 0; i < count; ++i) {
        REQUIRE(std::memcmp(dense.data() + 6 * i, single.data(),
                            6 * sizeof(RenderVertex)) == 0);
        REQUIRE(std::memcmp(dense.data() + 6 * count + 24 * i,
                            single.data() + 6, 24 * sizeof(RenderVertex)) == 0);
    }
}
