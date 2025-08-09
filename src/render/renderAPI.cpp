#include <json.hpp>

#include "api.h"
#include "gm.h"
#include "render.h"

DYCORE_API double DyCore_add_sprite_data(const char* spriteData) {
    auto j = nlohmann::json::parse(spriteData);
    SpriteData data = {.name = j["name"],
                       .size = {j["width"], j["height"]},
                       .uv0 = {j["uv00"], j["uv01"]},
                       .uv1 = {j["uv10"], j["uv11"]},
                       .paddingLR = j["paddingLR"],
                       .paddingTop = j["paddingTop"],
                       .paddingBottom = j["paddingBottom"]};
    SpriteDrawSetting setting;
    setting.type = static_cast<SPRITE_DRAW_TYPE>(j["type"].get<int>());
    std::vector<int> dataVec = j["data"].get<std::vector<int>>();
    for (size_t i = 0; i < dataVec.size(); i++) {
        setting.data[i] = dataVec[i];
    }
    data.drawSetting = setting;
    get_sprite_manager().add_sprite(data);
    return 0.0;
}

DYCORE_API double DyCore_get_note_rendering_vertex_buffer_bound() {
    return get_vertex_buffer_bound();
}

DYCORE_API double DyCore_render_active_notes(char* vertexBuffer, double nowTime,
                                             double noteSpeed, double state) {
    try {
        auto result =
            render_active_notes(vertexBuffer, nowTime, noteSpeed, state);
        return static_cast<double>(result);
    } catch (const std::exception& e) {
        throw_error_event(std::string("Error rendering active notes: ") +
                          e.what());
        return -1;
    }
}