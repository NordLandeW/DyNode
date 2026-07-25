#include "lualib.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

#include "LuaBridge/LuaBridge.h"
#include "gm.h"

void ll_gm_announcement(std::string str, std::string type, int lastTime) {
    std::transform(
        type.begin(), type.end(), type.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    GM_ANNOUNCEMENT_TYPE announcementType;
    if (type == "info" || type == "normal") {
        announcementType = GM_ANNOUNCEMENT_TYPE::ANNO_INFO;
    } else if (type == "warning" || type == "warn") {
        announcementType = GM_ANNOUNCEMENT_TYPE::ANNO_WARNING;
    } else if (type == "error") {
        announcementType = GM_ANNOUNCEMENT_TYPE::ANNO_ERROR;
    } else {
        throw std::invalid_argument("Unknown GameMaker announcement type: " +
                                    type);
    }

    if (lastTime < 0) {
        throw std::invalid_argument(
            "GameMaker announcement duration cannot be negative.");
    }

    gamemaker_announcement(announcementType, std::move(str), {}, lastTime);
}

void game_lualayer_openlibs(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("dynode")
        .beginNamespace("gm")
        .addFunction("announce", ll_gm_announcement)
        .endNamespace()
        .endNamespace();
}
