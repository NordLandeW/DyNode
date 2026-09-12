#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <format>
#include <limits>
#include <vector>

#include "activation.h"
#include "capacity.h"
#include "notePoolManager.h"
#include "render.h"

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
