#pragma once

#include <filesystem>
#include <json.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "luaext.h"

class LuaRunner {
   public:
    explicit LuaRunner(const std::filesystem::path& luaPath);
    ~LuaRunner();

    LuaRunner(const LuaRunner&) = delete;
    LuaRunner& operator=(const LuaRunner&) = delete;

    nlohmann::json start();
    nlohmann::json resume(nlohmann::json result);
    bool is_dead() const;

    luabridge::CppCoroutine<luabridge::LuaRef> gamemaker_execute(
        std::string functionName, std::vector<luabridge::LuaRef> args);

   private:
    struct LuaStateDeleter {
        void operator()(lua_State* lua) const;
    };

    nlohmann::json run();
    nlohmann::json finish_with_error(std::string error);

    std::unique_ptr<lua_State, LuaStateDeleter> lua;
    lua_State* luaCoroutine = nullptr;
    int luaCoroutineRef = LUA_NOREF;
    bool started = false;
    bool dead = false;
    bool waitingForGamemaker = false;
    std::optional<nlohmann::json> gamemakerResult;

    friend void game_lualayer_openlibs(LuaRunner& luaRunner);
};
