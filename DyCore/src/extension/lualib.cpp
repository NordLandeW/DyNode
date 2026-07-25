#include "lualib.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <utility>

#include "DyCore.h"
#include "LuaBridge/LuaBridge.h"
#include "editor.h"
#include "gm.h"
#include "version.h"

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

std::string ll_gm_prop_get_program_directory() {
    const std::filesystem::path programDirectory = get_program_path();
    if (programDirectory.empty()) {
        return {};
    }
    return std::filesystem::absolute(programDirectory).string();
}

std::string ll_gm_prop_get_working_directory() {
    return std::filesystem::current_path().string();
}

std::string ll_prop_get_game_version() {
    return DYNODE_VERSION;
}

bool ll_editor_is_ready() {
    return GMEditorManager::inst().is_ready();
}

int ll_editor_prop_get_editmode() {
    return GMEditorManager::inst().get_editmode();
}

void game_lualayer_openlibs(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("dynode")

        .addProperty("Version", ll_prop_get_game_version)
        .addProperty("ProgramDirectory", ll_gm_prop_get_program_directory)
        .addProperty("WorkingDirectory", ll_gm_prop_get_working_directory)

        .beginNamespace("gm")
        .addFunction("announce", ll_gm_announcement)
        .endNamespace()

        .beginNamespace("editor")
        .addFunction("is_ready", ll_editor_is_ready)
        .addProperty("editmode", ll_editor_prop_get_editmode)
        .endNamespace()

        .endNamespace();
}
