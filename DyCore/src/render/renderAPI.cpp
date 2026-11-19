#include <json.hpp>

#include "api.h"
#include "dynamite/fx.h"
#include "render.h"
#include "utils.h"

DYCORE_API double DyCore_add_sprite_data(const char* spriteData) {
    auto j = nlohmann::json::parse(spriteData);
    SpriteData data = {.name = j["name"],
                       .size = {j["width"], j["height"]},
                       .uv0 = {j["uv00"], j["uv01"]},
                       .uv1 = {j["uv10"], j["uv11"]},
                       .paddingLR = j["paddingLR"],
                       .paddingTop = j["paddingTop"],
                       .paddingBottom = j["paddingBottom"]};
    data.caculate_uv_values();

    SpriteDrawSetting setting{};
    setting.type = static_cast<SPRITE_DRAW_TYPE>(j["type"].get<int>());
    std::vector<int> dataVec = j["data"].get<std::vector<int>>();
    for (size_t i = 0; i < dataVec.size(); i++) {
        setting.data[i] = dataVec[i];
    }
    data.drawSetting = setting;
    get_sprite_manager().add_sprite(data);
    return 0.0;
}

DYCORE_API double DyCore_dynamite_hold_fill_vertices() {
    return static_cast<double>(dynamite_hold_fill_vertices());
}

DYCORE_API double DyCore_get_note_rendering_vertex_buffer_bound() {
    return get_vertex_buffer_bound();
}

DYCORE_API double DyCore_prepare_note_rendering() {
    try {
        return static_cast<double>(prepare_note_rendering());
    } catch (const std::exception& e) {
        print_debug_message(std::string("Error preparing note rendering: ") +
                            e.what());
        return -1;
    }
}

DYCORE_API double DyCore_render_active_notes(char* vertexBuffer, double nowTime,
                                             double noteSpeed, double state) {
    try {
        auto result =
            render_active_notes(vertexBuffer, nowTime, noteSpeed, state);
        return static_cast<double>(result);
    } catch (const std::exception& e) {
        print_debug_message(std::string("Error rendering active notes: ") +
                            e.what());
        return -1;
    }
}

DYCORE_API double DyCore_prepare_playback_notes(double nowTime,
                                                double frontSpeed,
                                                double sideSpeed,
                                                double dynamite) {
    try {
        return static_cast<double>(prepare_playback_notes(
            {nowTime, frontSpeed, sideSpeed, dynamite != 0}));
    } catch (const std::exception& e) {
        print_debug_message(std::string("Error preparing playback notes: ") +
                            e.what());
        return -1;
    }
}

DYCORE_API double DyCore_prepare_dynamite_fx(double chartTime,
                                             double editorClock, double setting,
                                             double mixers, double playback,
                                             double playing, double seeking) {
    try {
        return static_cast<double>(dynamite_fx().prepare(
            chartTime, editorClock, static_cast<int>(setting),
            static_cast<int>(mixers), playback != 0, playing != 0,
            seeking != 0));
    } catch (const std::exception& e) {
        print_debug_message(std::string("Error preparing Dynamite effects: ") +
                            e.what());
        return -1;
    }
}

DYCORE_API double DyCore_trigger_dynamite_hit(double side, double position,
                                              double width, double editorClock,
                                              double noteTime, double mixer,
                                              double light) {
    try {
        dynamite_fx().trigger_hit(
            {static_cast<int>(side), position, width, editorClock, mixer != 0},
            noteTime, light != 0);
        return 0;
    } catch (const std::exception& e) {
        print_debug_message(
            std::string("Error triggering Dynamite judgment: ") + e.what());
        return -1;
    }
}

DYCORE_API double DyCore_emit_dynamite_particles(double side, double center,
                                                 double width, double clock,
                                                 double mixer,
                                                 double chartStart,
                                                 double holdDuration) {
    try {
        dynamite_fx().particles.queue_hit(static_cast<int>(side), center, width,
                                          clock, mixer != 0, chartStart,
                                          holdDuration);
        return 0;
    } catch (const std::exception& e) {
        print_debug_message(std::string("Error queuing Dynamite particles: ") +
                            e.what());
        return -1;
    }
}

DYCORE_API double DyCore_dynamite_mixer_age(double side) {
    try {
        const double age = dynamite_fx().mixer_age(static_cast<int>(side));
        return std::isfinite(age) ? age : -1;
    } catch (const std::exception&) {
        return -1;
    }
}

DYCORE_API double DyCore_render_dynamite_fx(char* buffer, double pass) {
    try {
        return static_cast<double>(
            dynamite_fx().render(buffer, static_cast<int>(pass)));
    } catch (const std::exception& e) {
        print_debug_message(std::string("Error rendering Dynamite effects: ") +
                            e.what());
        return -1;
    }
}

DYCORE_API double DyCore_render_playback_notes(char* vertexBuffer,
                                               double state) {
    try {
        return static_cast<double>(
            render_playback_notes(vertexBuffer, static_cast<int>(state)));
    } catch (const std::exception& e) {
        print_debug_message(std::string("Error rendering playback notes: ") +
                            e.what());
        return -1;
    }
}
