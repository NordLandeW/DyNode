#include <doctest/doctest.h>

// clang-format off
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
// clang-format on

#include <string>

TEST_CASE("LuaRuntimeVersionIs55") {
    CHECK(LUA_VERSION_NUM == 505);
    CHECK(std::string(LUA_RELEASE) == "Lua 5.5.0");

    lua_State* lua = luaL_newstate();
    REQUIRE(lua != nullptr);

    luaL_openlibs(lua);
    lua_getglobal(lua, "_VERSION");
    REQUIRE(lua_isstring(lua, -1));
    CHECK(std::string(lua_tostring(lua, -1)) == "Lua 5.5");
    lua_pop(lua, 1);

    lua_close(lua);
}

TEST_CASE("LuaBridgeSupportsLua55") {
    CHECK(LUABRIDGE_VERSION == 301);

    lua_State* lua = luaL_newstate();
    REQUIRE(lua != nullptr);

    luaL_openlibs(lua);
    {
        luabridge::LuaRef table = luabridge::newTable(lua);
        table["answer"] = 42;
        REQUIRE(luabridge::setGlobal(lua, table, "bridge_test"));

        REQUIRE(luaL_dostring(lua, "return bridge_test.answer + 1") == LUA_OK);
        REQUIRE(lua_isinteger(lua, -1));
        CHECK(lua_tointeger(lua, -1) == 43);
        lua_pop(lua, 1);

        const auto global = luabridge::getGlobal(lua, "bridge_test");
        REQUIRE(global.isTable());
        const auto answer = global["answer"].cast<int>();
        REQUIRE(answer);
        CHECK(answer.value() == 42);
    }

    lua_close(lua);
}
