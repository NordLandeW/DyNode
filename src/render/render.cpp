#include "render.h"

#include <stdexcept>

void SpriteManager::add_sprite(const SpriteData& data) {
    sprites.emplace(data.name, data);
}

SpriteData SpriteManager::get_sprite(const std::string& name) const {
    auto it = sprites.find(name);
    if (it != sprites.end()) {
        return it->second;
    }
    throw std::runtime_error("Sprite not found");
}
