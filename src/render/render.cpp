#include "render.h"

#include <stdexcept>
#include <string>

#include "note.h"

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

string get_note_sprite_name(const Note& note) {
    static std::array<string, 3> sprite_names = {"sprNote", "sprChain",
                                                 "sprHold"};
    if (note.type < 0 || note.type >= sprite_names.size()) {
        throw std::out_of_range("Invalid note type");
    }
    return sprite_names[note.type];
}
SpriteData get_note_sprite(const Note& note) {
    return get_sprite_manager().get_sprite(get_note_sprite_name(note));
}

double get_note_pixel_width(const Note& note) {
    SpriteData sprite = get_note_sprite(note);
    return (note.side == 0 ? note.width * 300 : note.width * 150) - 30 +
           sprite.padding;
}

glm::vec2 get_note_pos(const Note& note, double noteSpeed, double nowTime) {
    glm::vec2 result;

    return result;
}

SpriteManager& get_sprite_manager() {
    static SpriteManager instance;
    return instance;
}
