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
