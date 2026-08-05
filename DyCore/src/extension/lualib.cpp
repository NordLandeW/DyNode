#include "lualib.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>

#include "DyCore.h"
#include "LuaBridge/LuaBridge.h"
#include "editor.h"
#include "gm.h"
#include "luaRunner.h"
#include "utils.h"
#include "version.h"

namespace {

std::string path_to_utf8(const std::filesystem::path& path) {
#ifdef _WIN32
    return wstringToUtf8(path.wstring());
#else
    return path.string();
#endif
}

constexpr const char* gmNamespaceSource = R"lua(
local exec = dynode.gm.exec

gm = setmetatable({ exec = exec }, {
    __index = function(_, functionName)
        return function(...)
            return exec(functionName, ...)
        end
    end,
})
)lua";

void open_gm_namespace(lua_State* L) {
    if (luaL_dostring(L, gmNamespaceSource) == LUA_OK) {
        return;
    }

    const char* message = lua_tostring(L, -1);
    const std::string error =
        message != nullptr ? message : "unknown Lua error";
    lua_pop(L, 1);
    throw std::runtime_error("Failed to register gm namespace: " + error);
}

}  // namespace

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

luabridge::CppCoroutine<luabridge::LuaRef> ll_gm_exec(LuaRunner& luaRunner,
                                                      std::string functionName,
                                                      lua_State* L) {
    std::vector<luabridge::LuaRef> args;
    const int argumentCount = lua_gettop(L);
    args.reserve(
        static_cast<size_t>(argumentCount > 1 ? argumentCount - 1 : 0));
    for (int index = 2; index <= argumentCount; ++index) {
        args.push_back(luabridge::LuaRef::fromStack(L, index));
    }

    return luaRunner.gamemaker_execute(std::move(functionName),
                                       std::move(args));
}

std::string ll_gm_prop_get_program_directory() {
    const std::filesystem::path programDirectory = get_program_path();
    if (programDirectory.empty()) {
        return {};
    }
    return path_to_utf8(std::filesystem::absolute(programDirectory));
}

std::string ll_gm_prop_get_working_directory() {
    return path_to_utf8(std::filesystem::current_path());
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

luabridge::CppCoroutine<luabridge::LuaRef> ll_editor_set_editmode(
    LuaRunner& luaRunner, luabridge::LuaRef editMode) {
    const auto convertedEditMode = editMode.cast<double>();
    if (!convertedEditMode) {
        throw std::invalid_argument("GameMaker editor mode must be a double.");
    }
    if (*convertedEditMode <= 0) {
        throw std::invalid_argument(
            "GameMaker editor mode cannot be non-positive.");
    }
    return luaRunner.gamemaker_execute("editor_set_editmode",
                                       {std::move(editMode)});
}

void game_lualayer_openlibs(LuaRunner& luaRunner) {
    lua_State* L = luaRunner.lua.get();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("dynode")

        .addProperty("Version", ll_prop_get_game_version)
        .addProperty("ProgramDirectory", ll_gm_prop_get_program_directory)
        .addProperty("WorkingDirectory", ll_gm_prop_get_working_directory)

        .beginNamespace("gm")
        .addFunction("announce", ll_gm_announcement)
        .addCoroutine("exec",
                      [&luaRunner](std::string functionName, lua_State* L) {
                          return ll_gm_exec(luaRunner, std::move(functionName),
                                            L);
                      })
        .endNamespace()

        .beginNamespace("editor")
        .addFunction("is_ready", ll_editor_is_ready)
        .addFunction("get_editmode", ll_editor_prop_get_editmode)
        .addCoroutine("set_editmode",
                      [&luaRunner](luabridge::LuaRef editMode) {
                          return ll_editor_set_editmode(luaRunner,
                                                        std::move(editMode));
                      })
        .endNamespace()

        .endNamespace();

    open_gm_namespace(L);
}
