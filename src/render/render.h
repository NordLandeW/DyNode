#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

const double HOLD_BG_LIGHTNESS = 0.3;
const int MULTITHREAD_RENDERING_THRESHOLD = 10000;

// NORMAL: A single, non-segmented sprite
//   data: none
// SEG_3: A 3-segment sprite drawing type, seg0/2 has fixed length, seg1 is
// stretchable
//   data: seg0 length, seg2 length
// SEG_5: A 5-segment sprite drawing type, seg0/2/4 has fixed length, seg1/3 is
// stretchable
//   data: seg0 length, seg2 length, seg4 length
// SLICE_9: A 9-slice sprite drawing type, empty at center.
//   data: left, right, top, bottom
// REPEAT_VERT: A vertically repeating sprite drawing type
// (stretched horizontally)
//   data: none
enum class SPRITE_DRAW_TYPE { NORMAL, SEG_3, SEG_5, SLICE_9, REPEAT_VERT };

enum class PIVOT { CENTER, BOTTOM_CENTER, TOP_CENTER };

struct SpriteDrawSetting {
    SPRITE_DRAW_TYPE type;
    int data[10];
};

struct SpriteData {
    std::string name;
    glm::vec2 size, uv0, uv1;
    int paddingLR;
    int paddingTop;
    int paddingBottom;
    SpriteDrawSetting drawSetting;

    glm::vec2 pos_to_uv(glm::vec2 pos) const {
        return pos / size * (uv1 - uv0) + uv0;
    }
    glm::vec2 uv_to_pos(glm::vec2 uv) const {
        return (uv - uv0) / (uv1 - uv0) * size;
    }
    glm::vec2 map_uv(glm::vec2 uv) const {
        return uv * (uv1 - uv0) + uv0;
    }
    glm::vec2 center() const {
        return (uv0 + uv1) / 2.0f;
    }
};

class SpriteManager {
   private:
    std::unordered_multimap<std::string, SpriteData> sprites;

   public:
    void add_sprite(const SpriteData& data);
    const SpriteData& get_sprite(const std::string& name) const;
};

size_t render_active_notes(char* const vertexBuffer, double nowTime,
                           double noteSpeed, int state);

size_t get_vertex_buffer_bound();

SpriteManager& get_sprite_manager();