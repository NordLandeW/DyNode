#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

struct SpriteData {
    std::string name;
    glm::vec2 size, uv0, uv1;

    glm::vec2 pos_to_uv(glm::vec2 pos) const {
        return pos / size * (uv1 - uv0) + uv0;
    }
};

class SpriteManager {
   private:
    std::unordered_multimap<std::string, SpriteData> sprites;

   public:
    void add_sprite(const SpriteData& data);
    SpriteData get_sprite(const std::string& name) const;
};