#include "render.h"

#include <algorithm>
#include <format>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/taskflow.hpp>

#include "activation.h"
#include "layout.h"
#include "note.h"
#include "notePoolManager.h"
#include "profile.h"
#include "utils.h"
#include "vertex.h"

void SpriteManager::add_sprite(const SpriteData& data) {
    print_debug_message(std::format(
        "add_sprite: name={}, size=({}, {}), uv0=({}, {}), uv1=({}, {}), "
        "paddingLR={}, paddingTop={}, paddingBottom={}, drawType={}",
        data.name, data.size.x, data.size.y, data.uv0.x, data.uv0.y, data.uv1.x,
        data.uv1.y, data.paddingLR, data.paddingTop, data.paddingBottom,
        static_cast<int>(data.drawSetting.type)));

    sprites[data.name] = data;
}

const SpriteData& SpriteManager::get_sprite(const std::string& name) const {
    auto it = sprites.find(name);
    if (it != sprites.end()) {
        return it->second;
    }
    throw std::runtime_error("Sprite not found");
}

string get_note_sprite_name(const Note& note) {
    static std::array<string, 3> sprite_names = {"sprNote", "sprChain",
                                                 "sprHoldEdge"};
    if (note.type < 0 || note.type >= sprite_names.size()) {
        throw std::out_of_range("Invalid note type");
    }
    return sprite_names[note.type];
}
SpriteData get_note_sprite(const Note& note) {
    return get_sprite_manager().get_sprite(get_note_sprite_name(note));
}

double get_note_pixel_width(const SpriteData& sprite, const Note& note) {
    return std::max((note.side == 0 ? note.width * 300 : note.width * 150) -
                        30 + sprite.paddingLR,
                    (double)sprite.size.x);
}

double get_note_pixel_height(const SpriteData& sprite, const Note& note,
                             double nowTime, double noteSpeed) {
    return std::max(0.0, noteSpeed * (note.time + note.lastTime -
                                      std::max(note.time, nowTime)) +
                             sprite.paddingBottom + sprite.paddingTop);
}

double pos_to_horzPos(double pos, int side) {
    if (side == 0)
        return BASE_RES_W / 2.0 + (pos - 2.5) * 300.0;
    else
        return BASE_RES_H / 2.0 + (2.5 - pos) * 150.0;
}

float time_to_vertPos(double time, double nowTime, double noteSpeed, int side) {
    if (side == 0) {
        return BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM -
               (time - nowTime) * noteSpeed;
    } else {
        return BASE_RES_W / 2.0 +
               (side == 1 ? -1 : 1) *
                   (BASE_RES_W / 2.0 - noteSpeed * (time - nowTime) -
                    JUDGE_LINE_SIDE_FROM_EDGE);
    }
}

glm::vec2 get_note_pos(const Note& note, double nowTime, double noteSpeed) {
    glm::vec2 result;

    if (note.side == 0) {
        result.x = pos_to_horzPos(note.position, note.side);
        result.y = time_to_vertPos(note.time, nowTime, noteSpeed, note.side);
    } else {
        result.y = pos_to_horzPos(note.position, note.side);
        result.x = time_to_vertPos(note.time, nowTime, noteSpeed, note.side);
    }

    return result;
}

double get_note_alpha(int side, glm::vec2 pos) {
    if (side == 0)
        return 1.0;
    else {
        return std::clamp(
            std::lerp(0.25, 1.0,
                      std::abs(pos.x - BASE_RES_W / 2.0) / (0.3 * BASE_RES_W)),
            0.0, 1.0);
    }
}

double get_note_rotation(int side) {
    switch (side) {
        default:
        case 0:
            return 0.0;
        case 1:
            return 270.0;
        case 2:
            return 90.0;
    }
}

// Draw a sprite with the given position, size, and rotation
void draw_sprite(char*& vertBuf, const SpriteData& sprite, const PIVOT pivot,
                 glm::vec2 position, glm::vec2 size, double rotation,
                 glm::ivec4 color) {
    // Calculate quad range.
    glm::vec2 halfSize = size / 2.0f;
    glm::vec2 leftUp, rightDown;
    if (pivot == PIVOT::CENTER) {
        leftUp = position - halfSize;
        rightDown = position + halfSize;
    } else if (pivot == PIVOT::BOTTOM_CENTER) {
        leftUp = {position.x - halfSize.x, position.y - size.y};
        rightDown = {position.x + halfSize.x, position.y};
    } else if (pivot == PIVOT::TOP_CENTER) {
        leftUp = {position.x - halfSize.x, position.y};
        rightDown = {position.x + halfSize.x, position.y + size.y};
    }
    glm::vec2 leftDown = {leftUp.x, rightDown.y};
    glm::vec2 rightUp = {rightDown.x, leftUp.y};

    // Rotation logic
    const float angle = glm::radians(-rotation);
    const float s = sin(angle);
    const float c = cos(angle);
    auto rotate = [&](glm::vec2 p) {
        p -= position;
        return glm::vec2(p.x * c - p.y * s, p.x * s + p.y * c) + position;
    };

    // Draw the sprite using the calculated quad range.
    auto setting = sprite.drawSetting;
    switch (setting.type) {
        // Should enable tex_repeat setting.
        case SPRITE_DRAW_TYPE::REPEAT_VERT: {
            float current_y = 0.0f;
            while (current_y < size.y) {
                const float remaining_h = size.y - current_y;
                const float quad_h =
                    std::min(remaining_h, (float)sprite.size.y);

                const glm::vec2 quad_tl = leftUp + glm::vec2(0.0f, current_y);
                const glm::vec2 quad_tr = rightUp + glm::vec2(0.0f, current_y);
                const glm::vec2 quad_bl =
                    leftUp + glm::vec2(0.0f, current_y + quad_h);
                const glm::vec2 quad_br =
                    rightUp + glm::vec2(0.0f, current_y + quad_h);

                const glm::vec2 uv_tl = sprite.map_uv({0.0f, 0.0f});
                const glm::vec2 uv_tr = sprite.map_uv({1.0f, 0.0f});
                const glm::vec2 uv_bl =
                    sprite.map_uv({0.0f, quad_h / sprite.size.y});
                const glm::vec2 uv_br =
                    sprite.map_uv({1.0f, quad_h / sprite.size.y});

                vertex_quad_write(vertBuf, rotate(quad_tl), rotate(quad_tr),
                                  rotate(quad_bl), rotate(quad_br), uv_tl,
                                  uv_tr, uv_bl, uv_br, color);

                current_y += quad_h;
            }
            break;
        }
        case SPRITE_DRAW_TYPE::NORMAL: {
            vertex_quad_write(
                vertBuf, rotate(leftUp), rotate(rightUp), rotate(leftDown),
                rotate(rightDown), sprite.map_uv({0.0, 0.0}),
                sprite.map_uv({1.0, 0.0}), sprite.map_uv({0.0, 1.0}),
                sprite.map_uv({1.0, 1.0}), color);
            break;
        }
        case SPRITE_DRAW_TYPE::SEG_3: {
            const float seg0_w = setting.data[0];
            const float seg2_w = setting.data[1];
            const float seg1_uv_w = sprite.size.x - seg0_w - seg2_w;
            const float seg1_screen_w = size.x - seg0_w - seg2_w;

            // Draw seg-0
            vertex_quad_write(vertBuf, rotate({leftUp.x, leftUp.y}),
                              rotate({leftUp.x + seg0_w, leftUp.y}),
                              rotate({leftDown.x, leftDown.y}),
                              rotate({leftDown.x + seg0_w, leftDown.y}),
                              sprite.pos_to_uv({0, 0}),
                              sprite.pos_to_uv({seg0_w, 0}),
                              sprite.pos_to_uv({0, sprite.size.y}),
                              sprite.pos_to_uv({seg0_w, sprite.size.y}), color);
            // Draw seg-1
            vertex_quad_write(
                vertBuf, rotate({leftUp.x + seg0_w, leftUp.y}),
                rotate({leftUp.x + seg0_w + seg1_screen_w, leftUp.y}),
                rotate({leftDown.x + seg0_w, leftDown.y}),
                rotate({leftDown.x + seg0_w + seg1_screen_w, leftDown.y}),
                sprite.pos_to_uv({seg0_w, 0}),
                sprite.pos_to_uv({seg0_w + seg1_uv_w, 0}),
                sprite.pos_to_uv({seg0_w, sprite.size.y}),
                sprite.pos_to_uv({seg0_w + seg1_uv_w, sprite.size.y}), color);
            // Draw seg-2
            vertex_quad_write(
                vertBuf, rotate({rightUp.x - seg2_w, rightUp.y}),
                rotate({rightUp.x, rightUp.y}),
                rotate({rightDown.x - seg2_w, rightDown.y}),
                rotate({rightDown.x, rightDown.y}),
                sprite.pos_to_uv({sprite.size.x - seg2_w, 0}),
                sprite.pos_to_uv({sprite.size.x, 0}),
                sprite.pos_to_uv({sprite.size.x - seg2_w, sprite.size.y}),
                sprite.pos_to_uv({sprite.size.x, sprite.size.y}), color);
            break;
        }
        case SPRITE_DRAW_TYPE::SEG_5: {
            const float seg0_w = setting.data[0];
            const float seg2_w = setting.data[1];
            const float seg4_w = setting.data[2];
            const float seg13_uv_w = sprite.size.x - seg0_w - seg2_w - seg4_w;
            const float seg13_screen_w = size.x - seg0_w - seg2_w - seg4_w;
            const float seg1_uv_w = seg13_uv_w / 2.0f;
            const float seg3_uv_w = seg13_uv_w / 2.0f;
            const float seg1_screen_w = seg13_screen_w / 2.0f;
            const float seg3_screen_w = seg13_screen_w / 2.0f;

            float current_x = leftUp.x;
            float current_uv_x = 0;

            // Draw seg-0
            vertex_quad_write(
                vertBuf, rotate({current_x, leftUp.y}),
                rotate({current_x + seg0_w, leftUp.y}),
                rotate({current_x, leftDown.y}),
                rotate({current_x + seg0_w, leftDown.y}),
                sprite.pos_to_uv({current_uv_x, 0}),
                sprite.pos_to_uv({current_uv_x + seg0_w, 0}),
                sprite.pos_to_uv({current_uv_x, sprite.size.y}),
                sprite.pos_to_uv({current_uv_x + seg0_w, sprite.size.y}),
                color);
            current_x += seg0_w;
            current_uv_x += seg0_w;

            // Draw seg-1
            vertex_quad_write(
                vertBuf, rotate({current_x, leftUp.y}),
                rotate({current_x + seg1_screen_w, leftUp.y}),
                rotate({current_x, leftDown.y}),
                rotate({current_x + seg1_screen_w, leftDown.y}),
                sprite.pos_to_uv({current_uv_x, 0}),
                sprite.pos_to_uv({current_uv_x + seg1_uv_w, 0}),
                sprite.pos_to_uv({current_uv_x, sprite.size.y}),
                sprite.pos_to_uv({current_uv_x + seg1_uv_w, sprite.size.y}),
                color);
            current_x += seg1_screen_w;
            current_uv_x += seg1_uv_w;

            // Draw seg-2
            vertex_quad_write(
                vertBuf, rotate({current_x, leftUp.y}),
                rotate({current_x + seg2_w, leftUp.y}),
                rotate({current_x, leftDown.y}),
                rotate({current_x + seg2_w, leftDown.y}),
                sprite.pos_to_uv({current_uv_x, 0}),
                sprite.pos_to_uv({current_uv_x + seg2_w, 0}),
                sprite.pos_to_uv({current_uv_x, sprite.size.y}),
                sprite.pos_to_uv({current_uv_x + seg2_w, sprite.size.y}),
                color);
            current_x += seg2_w;
            current_uv_x += seg2_w;

            // Draw seg-3
            vertex_quad_write(
                vertBuf, rotate({current_x, leftUp.y}),
                rotate({current_x + seg3_screen_w, leftUp.y}),
                rotate({current_x, leftDown.y}),
                rotate({current_x + seg3_screen_w, leftDown.y}),
                sprite.pos_to_uv({current_uv_x, 0}),
                sprite.pos_to_uv({current_uv_x + seg3_uv_w, 0}),
                sprite.pos_to_uv({current_uv_x, sprite.size.y}),
                sprite.pos_to_uv({current_uv_x + seg3_uv_w, sprite.size.y}),
                color);
            current_x += seg3_screen_w;
            current_uv_x += seg3_uv_w;

            // Draw seg-4
            vertex_quad_write(
                vertBuf, rotate({current_x, leftUp.y}),
                rotate({current_x + seg4_w, leftUp.y}),
                rotate({current_x, leftDown.y}),
                rotate({current_x + seg4_w, leftDown.y}),
                sprite.pos_to_uv({current_uv_x, 0}),
                sprite.pos_to_uv({current_uv_x + seg4_w, 0}),
                sprite.pos_to_uv({current_uv_x, sprite.size.y}),
                sprite.pos_to_uv({current_uv_x + seg4_w, sprite.size.y}),
                color);
            break;
        }
        case SPRITE_DRAW_TYPE::SLICE_9: {
            const float left = setting.data[0];
            const float right = setting.data[1];
            const float top = setting.data[2];
            const float bottom = setting.data[3];

            const float x_coords[] = {leftUp.x, leftUp.x + left,
                                      rightUp.x - right, rightUp.x};
            const float y_coords[] = {leftUp.y, leftUp.y + top,
                                      leftDown.y - bottom, leftDown.y};

            const float uv_x_coords[] = {0, left, sprite.size.x - right,
                                         sprite.size.x};
            const float uv_y_coords[] = {0, top, sprite.size.y - bottom,
                                         sprite.size.y};

            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j)
                    if (!(i == 1 && j == 1)) {
                        vertex_quad_write(
                            vertBuf, rotate({x_coords[j], y_coords[i]}),
                            rotate({x_coords[j + 1], y_coords[i]}),
                            rotate({x_coords[j], y_coords[i + 1]}),
                            rotate({x_coords[j + 1], y_coords[i + 1]}),
                            sprite.pos_to_uv({uv_x_coords[j], uv_y_coords[i]}),
                            sprite.pos_to_uv(
                                {uv_x_coords[j + 1], uv_y_coords[i]}),
                            sprite.pos_to_uv(
                                {uv_x_coords[j], uv_y_coords[i + 1]}),
                            sprite.pos_to_uv(
                                {uv_x_coords[j + 1], uv_y_coords[i + 1]}),
                            color);
                    }
            }
            break;
        }
    }
}

// Notice that area indicates (x, y, w, h) in sprite space.
void draw_sprite_part(char*& vertBuf, const SpriteData& sprite,
                      const PIVOT pivot, glm::vec2 position, glm::vec4 area,
                      double rotation, glm::ivec4 color) {
    // area = (x, y, w, h)
    glm::vec2 size = {area.z, area.w};

    // Calculate quad range.
    glm::vec2 halfSize = size / 2.0f;
    glm::vec2 leftUp, rightDown;
    if (pivot == PIVOT::CENTER) {
        leftUp = position - halfSize;
        rightDown = position + halfSize;
    } else if (pivot == PIVOT::BOTTOM_CENTER) {
        leftUp = {position.x - halfSize.x, position.y - size.y};
        rightDown = {position.x + halfSize.x, position.y};
    } else {
        throw std::runtime_error("Unsupported pivot type");
    }
    glm::vec2 leftDown = {leftUp.x, rightDown.y};
    glm::vec2 rightUp = {rightDown.x, leftUp.y};

    // Rotation logic
    const float angle = glm::radians(-rotation);
    const float s = sin(angle);
    const float c = cos(angle);
    auto rotate = [&](glm::vec2 p) {
        p -= position;
        return glm::vec2(p.x * c - p.y * s, p.x * s + p.y * c) + position;
    };

    // Calculate UVs for the specified part
    glm::vec2 uv_lu = sprite.pos_to_uv({area.x, area.y});
    glm::vec2 uv_ru = sprite.pos_to_uv({area.x + area.z, area.y});
    glm::vec2 uv_ld = sprite.pos_to_uv({area.x, area.y + area.w});
    glm::vec2 uv_rd = sprite.pos_to_uv({area.x + area.z, area.y + area.w});

    // Draw the sprite part using the calculated quad range and UVs.
    vertex_quad_write(vertBuf, rotate(leftUp), rotate(rightUp),
                      rotate(leftDown), rotate(rightDown), uv_lu, uv_ru, uv_ld,
                      uv_rd, color);
}

// 1 Quad = 6 Vertices = 120 Bytes
constexpr size_t BYTES_PER_QUAD = 120;

size_t get_sprite_max_bytes(const SpriteData& sprite) {
    try {
        if (sprite.drawSetting.type == SPRITE_DRAW_TYPE::REPEAT_VERT) {
            // The maximum render length is approximately the maximum screen
            // dimension plus some padding.
            float tile_y = std::max(1.0f, (float)sprite.size.y);
            float max_len = std::max(BASE_RES_W, BASE_RES_H) + 3.0f * tile_y;
            return static_cast<size_t>(std::ceil(max_len / tile_y)) *
                   BYTES_PER_QUAD;
        }
        if (sprite.drawSetting.type == SPRITE_DRAW_TYPE::SLICE_9)
            return 9 * BYTES_PER_QUAD;
        if (sprite.drawSetting.type == SPRITE_DRAW_TYPE::SEG_5)
            return 5 * BYTES_PER_QUAD;
        if (sprite.drawSetting.type == SPRITE_DRAW_TYPE::SEG_3)
            return 3 * BYTES_PER_QUAD;
        return 1 * BYTES_PER_QUAD;
    } catch (...) {
        // Safe fallback value when the sprite atlas is not yet ready.
        return 256 * BYTES_PER_QUAD;
    }
}
size_t get_sprite_max_bytes(const std::string& name) {
    return get_sprite_max_bytes(get_sprite_manager().get_sprite(name));
}

// For param state:
//   0: Render addition bg
//   1: Render hold bg
//   2: Render other parts
size_t render_active_notes(char* const vertexBuffer, double nowTime,
                           double noteSpeed, int state) {
    PROFILE_SCOPE(std::format("Render Active Notes (State {})", state));
    const bool detailedProfiling = false;
    char* ptr = vertexBuffer;

    // Get active notes list.
    const auto& actMan = get_note_activation_manager();
    const auto& activeNotes = actMan.get_active_notes();
    const auto& activeHolds = actMan.get_active_holds();
    const auto& lastingHolds = actMan.get_lasting_holds();

    // Get sprites.
    const auto& spriteMan = get_sprite_manager();
    const auto& tapNoteSprite = spriteMan.get_sprite("sprNote");
    const auto& chainNoteSprite = spriteMan.get_sprite("sprChain");
    const auto& holdEdgeSprite = spriteMan.get_sprite("sprHoldEdge");
    const auto& holdBarSprite = spriteMan.get_sprite("sprHold");
    const auto& holdBgSprite = spriteMan.get_sprite("sprHoldGrey");

    auto render_normal = [&](char*& vertBuf, const Note& note) {
        glm::vec2 pos = get_note_pos(note, nowTime, noteSpeed);
        double alpha = get_note_alpha(note.side, pos);
        double rot = get_note_rotation(note.side);
        PIVOT pivot = PIVOT::CENTER;
        const SpriteData& spriteData =
            note.type == 0 ? tapNoteSprite : chainNoteSprite;
        glm::vec2 size = spriteData.size;
        size.x = get_note_pixel_width(spriteData, note);

        draw_sprite(vertBuf, spriteData, pivot, pos, size, rot,
                    {255, 255, 255, static_cast<int>(alpha * 255)});
    };

    // Render hold notes.
    // renderType:
    //   0: render addition bg
    //   1: render bg
    //   2: render edge
    auto render_hold = [&](char*& vertBuf, const Note& note, int renderType) {
        static auto in_between = [](double value, double limit1,
                                    double limit2) {
            if (limit1 > limit2)
                std::swap(limit1, limit2);
            return value >= limit1 && value <= limit2;
        };

        glm::vec2 position = get_note_pos(note, nowTime, noteSpeed);
        const double alpha = get_note_alpha(note.side, position);
        const double rotation = get_note_rotation(note.side);
        const auto& edgeSprite = holdEdgeSprite;
        const auto& barSprite = holdBarSprite;
        const double spriteTileHeight = barSprite.size.y;

        double edgeLength =
            get_note_pixel_height(edgeSprite, note, nowTime, noteSpeed);
        if (edgeLength < edgeSprite.size.y && note.time < nowTime)
            return;

        edgeLength = std::max(edgeLength, (double)edgeSprite.size.y);
        double barLength =
            edgeLength - edgeSprite.paddingTop - edgeSprite.paddingBottom;

        // Determine the main axis and screen dimension based on the note's
        // side.
        const bool isVertical = (note.side == 0);
        const double screenDim = isVertical ? BASE_RES_H : BASE_RES_W;

        // If the bar is extremely long (e.g., starts on-screen but
        // extends far off-screen), shorten it to a more reasonable length.
        const double maxLengthThreshold = screenDim + 2 * spriteTileHeight;
        if (barLength > maxLengthThreshold) {
            double excessLength = std::floor((barLength - maxLengthThreshold) /
                                             spriteTileHeight) *
                                  spriteTileHeight;
            barLength -= excessLength;
        }

        // Update edgeLength based on the potentially shortened barLength.
        edgeLength =
            barLength + edgeSprite.paddingTop + edgeSprite.paddingBottom;

        // Clamp the edge length.
        edgeLength = std::min(edgeLength, screenDim + 3 * spriteTileHeight);

        if (note.side == 0) {
            position.y = std::min(
                position.y, float(BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM));
        } else if (note.side == 1) {
            position.x = std::max(position.x, float(JUDGE_LINE_SIDE_FROM_EDGE));
        } else {
            position.x = std::min(
                position.x, float(BASE_RES_W - JUDGE_LINE_SIDE_FROM_EDGE));
        }

        switch (renderType) {
            case 0:
            case 1: {
                if (barLength > 0) {
                    glm::vec2 size = {get_note_pixel_width(edgeSprite, note) -
                                          edgeSprite.paddingLR,
                                      barLength};
                    PIVOT pivot = PIVOT::TOP_CENTER;
                    if (note.side == 0) {
                        position.y -= size.y;
                    } else {
                        position.x += size.y * (note.side == 1 ? 1 : -1);
                    }
                    if (renderType == 1) {
                        draw_sprite(
                            vertBuf, barSprite, pivot, position, size, rotation,
                            {255, 255, 255, static_cast<int>(alpha * 255)});
                    } else {
                        const auto& bgSprite = holdBgSprite;
                        draw_sprite(vertBuf, bgSprite, pivot, position, size,
                                    rotation,
                                    {0, 255, 0,
                                     static_cast<int>(alpha * 255 *
                                                      HOLD_BG_LIGHTNESS)});
                    }
                }
                break;
            }
            case 2: {
                if (edgeLength > 0) {
                    glm::vec2 size = {get_note_pixel_width(edgeSprite, note),
                                      edgeLength};
                    PIVOT pivot = PIVOT::BOTTOM_CENTER;
                    if (note.side == 0) {
                        position.y += edgeSprite.paddingBottom;
                    } else {
                        position.x += edgeSprite.paddingBottom *
                                      (note.side == 1 ? -1 : 1);
                    }
                    draw_sprite(vertBuf, edgeSprite, pivot, position, size,
                                rotation,
                                {255, 255, 255, static_cast<int>(alpha * 255)});
                }
                break;
            }
            default:
                break;
        }
    };

    if (state == 0) {
        // Render additional background
        for (const auto& [time, noteID] : lastingHolds) {
            const auto& note = get_note_pool_manager().get_note_unsafe(noteID);
            render_hold(ptr, note, 0);
        }
        return ptr - vertexBuffer;
    }

    if (state == 1) {
        // Render hold bg.
        for (const auto& [time, noteID] : activeHolds) {
            const auto& note = get_note_pool_manager().get_note_unsafe(noteID);

            render_hold(ptr, note, 1);
        }
        return ptr - vertexBuffer;
    }

    int concurrency = hardware_concurrency();

    static std::array<std::byte, 128 * 1024 * 1024> baseBuffer;
    static std::pmr::monotonic_buffer_resource monoBuffer{
        baseBuffer.data(), baseBuffer.size(), std::pmr::new_delete_resource()};
    monoBuffer.release();

    struct Task {
        char* buffer;
        int l, r;
        char* ptr;
    };

    if (concurrency > 1 &&
        activeNotes.size() > MULTITHREAD_RENDERING_THRESHOLD) {
        static tf::Executor executor;

        auto render_pass = [&](const NoteActivationManager::ActiveLists&
                                   activeList,
                               int type) {
            PROFILE_SCOPE_CONDITIONAL("RenderPass#" + std::to_string(type),
                                      detailedProfiling);
            tf::Taskflow taskflow;
            std::vector<Task> tasks;
            {
                PROFILE_SCOPE_CONDITIONAL(
                    "RenderPassTaskGeneration#" + std::to_string(type),
                    detailedProfiling);
                int blockSize = std::ceil(
                    static_cast<double>(activeList.size()) / concurrency);
                for (int i = 0; i * blockSize < activeList.size(); i++) {
                    int l = i * blockSize;
                    int r = std::min((i + 1) * blockSize - 1,
                                     static_cast<int>(activeList.size() - 1));

                    if (l > r)
                        continue;

                    size_t bytes_per_item = 2040;
                    if (type == 0)
                        bytes_per_item = get_sprite_max_bytes(tapNoteSprite);
                    else if (type == 1)
                        bytes_per_item = get_sprite_max_bytes(chainNoteSprite);
                    else if (type == 2)
                        bytes_per_item = get_sprite_max_bytes(holdEdgeSprite);

                    size_t bytes = 1ll * (r - l + 1) * bytes_per_item;
                    char* alloc_buf = static_cast<char*>(
                        monoBuffer.allocate(bytes, alignof(char)));
                    tasks.push_back(Task{alloc_buf, l, r, alloc_buf});
                }

                taskflow.for_each(tasks.begin(), tasks.end(), [&](Task& task) {
                    for (int i = task.l; i <= task.r; i++) {
                        const auto& [time, noteID] = activeList[i];
                        const auto& note =
                            get_note_pool_manager().get_note_unsafe(noteID);
                        if (note.type != type)
                            continue;
                        switch (note.get_note_type()) {
                            case NOTE_TYPE::NORMAL:
                            case NOTE_TYPE::CHAIN:
                                render_normal(task.ptr, note);
                                break;
                            case NOTE_TYPE::HOLD:
                                render_hold(task.ptr, note, 2);
                                break;
                            default:
                                break;
                        }
                    }
                });
            }
            {
                PROFILE_SCOPE_CONDITIONAL(
                    "RenderPassTaskExecution#" + std::to_string(type),
                    detailedProfiling);
                executor.run(taskflow).wait();
            }
            {
                PROFILE_SCOPE_CONDITIONAL(
                    "RenderPassTaskBufferCopy#" + std::to_string(type),
                    detailedProfiling);
                for (auto& task : tasks) {
                    memcpy(ptr, task.buffer, task.ptr - (char*)task.buffer);
                    ptr += task.ptr - (char*)task.buffer;
                }
            }
        };
        render_pass(activeHolds, 2);
        render_pass(activeNotes, 0);
        render_pass(activeNotes, 1);
    } else {
        // Render hold edges.
        for (const auto& [time, noteID] : activeHolds) {
            const auto& note = get_note_pool_manager().get_note_unsafe(noteID);
            if (note.get_note_type() != NOTE_TYPE::HOLD)
                continue;

            render_hold(ptr, note, 2);
        }

        // Render normal notes.
        for (const auto& [time, noteID] : activeNotes) {
            const auto& note = get_note_pool_manager().get_note_unsafe(noteID);
            if (note.get_note_type() != NOTE_TYPE::NORMAL)
                continue;

            render_normal(ptr, note);
        }

        // Render chain notes.
        for (const auto& [time, noteID] : activeNotes) {
            const auto& note = get_note_pool_manager().get_note_unsafe(noteID);
            if (note.get_note_type() != NOTE_TYPE::CHAIN)
                continue;

            render_normal(ptr, note);
        }
    }

    return ptr - vertexBuffer;
}

size_t get_vertex_buffer_bound() {
    const auto& actMan = get_note_activation_manager();
    const auto& activeNotes = actMan.get_active_notes();
    const auto& activeHolds = actMan.get_active_holds();
    const auto& lastingHolds = actMan.get_lasting_holds();

    // Pre-calculate the maximum quad cost for all sprites.
    size_t bgBytes = get_sprite_max_bytes("sprHoldGrey");
    size_t barBytes = get_sprite_max_bytes("sprHold");
    size_t edgeBytes = get_sprite_max_bytes("sprHoldEdge");
    size_t tapBytes = std::max(get_sprite_max_bytes("sprNote"),
                               get_sprite_max_bytes("sprChain"));

    size_t total_bound = 0;

    // State 0
    total_bound += lastingHolds.size() * bgBytes;

    // State 1
    total_bound += activeHolds.size() * barBytes;

    // State 2
    total_bound += activeHolds.size() * edgeBytes;
    total_bound += (activeNotes.size() - activeHolds.size()) * tapBytes;

    return total_bound + 1024 * BYTES_PER_QUAD;
}

SpriteManager& get_sprite_manager() {
    static SpriteManager instance;
    return instance;
}
