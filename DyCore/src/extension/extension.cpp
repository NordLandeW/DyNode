#include "extension.h"

#include <memory>
#include <stdexcept>
#include <string>

#include "luaext.h"
#include "lualib.h"

void run_lua_script(const std::filesystem::path& scriptPath) {
    std::unique_ptr<lua_State, decltype(&lua_close)> lua(luaL_newstate(),
                                                         &lua_close);
    if (!lua) {
        throw std::runtime_error("Failed to create Lua state.");
    }

    auto L = lua.get();

    // Initialize libraries.
    luaL_openlibs(L);
    game_lualayer_openlibs(L);

    const std::string scriptPathString = scriptPath.string();
    const int result = luaL_dofile(L, scriptPathString.c_str());
    if (result != LUA_OK) {
        const char* luaError = lua_tostring(L, -1);
        throw std::runtime_error(
            "Failed to run Lua script '" + scriptPathString +
            "': " + (luaError ? luaError : "unknown Lua error"));
    }
}

void run_lua_script() {
    run_lua_script(std::filesystem::current_path() / "script.lua");
}
