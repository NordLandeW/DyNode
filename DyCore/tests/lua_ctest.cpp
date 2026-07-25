#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <json.hpp>
#include <stdexcept>
#include <string>

#include "DyCore.h"
#include "editor.h"
#include "extension.h"
#include "gm.h"
#include "luaext.h"
#include "lualib.h"

extern "C" double DyCore_has_async_event();
extern "C" const char* DyCore_get_async_event();

namespace {

std::filesystem::path make_lua_temp_dir() {
    const auto ticks = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    auto dir = std::filesystem::temp_directory_path() /
               ("dynode_lua_script_test_" + std::to_string(ticks));
    std::filesystem::create_directories(dir);
    return dir;
}

}  // namespace

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

TEST_CASE("LuaGmAnnouncementQueuesEvent") {
    while (DyCore_has_async_event() > 0) {
        DyCore_get_async_event();
    }

    lua_State* lua = luaL_newstate();
    REQUIRE(lua != nullptr);

    luaL_openlibs(lua);
    game_lualayer_openlibs(lua);
    REQUIRE(luaL_dostring(
                lua,
                "dynode.gm.announce('Lua announcement', 'warning', 4321)") ==
            LUA_OK);
    lua_close(lua);

    REQUIRE(DyCore_has_async_event() > 0);

    const auto event = nlohmann::json::parse(DyCore_get_async_event());
    const auto content = nlohmann::json::parse(
        event.at("content").get_ref<const std::string&>());

    CHECK(event.at("status") ==
          static_cast<int>(GM_ANNOUNCEMENT_TYPE::ANNO_WARNING));
    CHECK(content.at("msg") == "Lua announcement");
    CHECK(content.at("args").empty());
    CHECK(content.at("lastTime") == 4321);
}

TEST_CASE("LuaGmDirectoryPropertiesReflectProcessDirectories") {
    namespace fs = std::filesystem;

    const fs::path originalWorkingDirectory = fs::current_path();
    const fs::path dir = make_lua_temp_dir();

    try {
        fs::current_path(dir);

        const std::string programDirectory =
            ll_gm_prop_get_program_directory();
        const std::string workingDirectory =
            ll_gm_prop_get_working_directory();

        const fs::path configuredProgramDirectory = get_program_path();
        const std::string expectedProgramDirectory =
            configuredProgramDirectory.empty()
                ? std::string()
                : fs::absolute(configuredProgramDirectory).string();

        CHECK(programDirectory == expectedProgramDirectory);
        if (!programDirectory.empty()) {
            CHECK(fs::path(programDirectory).is_absolute());
        }
        CHECK(workingDirectory == dir.string());
        CHECK(fs::path(workingDirectory).is_absolute());

        lua_State* lua = luaL_newstate();
        REQUIRE(lua != nullptr);

        luaL_openlibs(lua);
        game_lualayer_openlibs(lua);
        REQUIRE(luaL_dostring(
                    lua,
                    "return dynode.ProgramDirectory, "
                    "dynode.WorkingDirectory, dynode.Version") == LUA_OK);
        REQUIRE(lua_isstring(lua, -3));
        REQUIRE(lua_isstring(lua, -2));
        REQUIRE(lua_isstring(lua, -1));
        CHECK(std::string(lua_tostring(lua, -3)) == programDirectory);
        CHECK(std::string(lua_tostring(lua, -2)) == workingDirectory);
        CHECK(std::string(lua_tostring(lua, -1)) == ll_prop_get_game_version());
        lua_close(lua);

        fs::current_path(originalWorkingDirectory);
    } catch (...) {
        std::error_code ec;
        fs::current_path(originalWorkingDirectory, ec);
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaEditorGetterReflectsSynchronizedGameMakerState") {
    gmeditor_sync_states({{"editMode", 4}});

    lua_State* lua = luaL_newstate();
    REQUIRE(lua != nullptr);

    luaL_openlibs(lua);
    game_lualayer_openlibs(lua);
    REQUIRE(luaL_dostring(lua, "return dynode.editor.get_editmode()") ==
            LUA_OK);
    REQUIRE(lua_isinteger(lua, -1));
    CHECK(lua_tointeger(lua, -1) == 4);
    lua_close(lua);
}

TEST_CASE("LuaEditorSetEditmodeQueuesGameMakerExecute") {
    while (DyCore_has_async_event() > 0) {
        DyCore_get_async_event();
    }

    lua_State* lua = luaL_newstate();
    REQUIRE(lua != nullptr);

    luaL_openlibs(lua);
    game_lualayer_openlibs(lua);

    REQUIRE(luaL_dostring(lua, "dynode.editor.set_editmode(0)") == LUA_OK);
    REQUIRE(luaL_dostring(lua, "dynode.editor.set_editmode(5)") == LUA_OK);

    for (const int expectedMode : {0, 5}) {
        REQUIRE(DyCore_has_async_event() > 0);
        const auto event = nlohmann::json::parse(DyCore_get_async_event());
        const auto content = nlohmann::json::parse(
            event.at("content").get_ref<const std::string&>());

        CHECK(event.at("type") == GM_EXECUTE);
        CHECK(content.at("name") == "editor_set_editmode");
        CHECK(content.at("args") == nlohmann::json::array({expectedMode}));
    }
    CHECK(DyCore_has_async_event() == 0);

    CHECK(luaL_dostring(lua, "dynode.editor.set_editmode(-1)") != LUA_OK);
    const char* error = lua_tostring(lua, -1);
    REQUIRE(error != nullptr);
    CHECK(std::string(error).find("cannot be negative") != std::string::npos);
    lua_pop(lua, 1);
    CHECK(DyCore_has_async_event() == 0);

    lua_close(lua);
}

TEST_CASE("RunLuaScriptUsesWorkingDirectory") {
    namespace fs = std::filesystem;

    const fs::path dir = make_lua_temp_dir();
    const fs::path scriptPath = dir / "script.lua";
    const fs::path resultPath = dir / "result.txt";
    const fs::path originalWorkingDirectory = fs::current_path();

    {
        std::ofstream script(scriptPath, std::ios::binary | std::ios::trunc);
        REQUIRE(script.is_open());
        script << "local output = assert(io.open('result.txt', 'w'))\n"
               << "output:write('Lua script ran')\n"
               << "output:close()\n";
    }

    try {
        fs::current_path(dir);
        run_lua_script();

        std::ifstream result(resultPath, std::ios::binary);
        REQUIRE(result.is_open());
        CHECK(std::string(std::istreambuf_iterator<char>(result),
                          std::istreambuf_iterator<char>()) ==
              "Lua script ran");

        {
            std::ofstream script(scriptPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "error('intentional Lua failure')\n";
        }
        CHECK_THROWS_WITH_AS(
            run_lua_script(),
            doctest::Contains("intentional Lua failure"), std::runtime_error);

        fs::current_path(originalWorkingDirectory);
    } catch (...) {
        std::error_code ec;
        fs::current_path(originalWorkingDirectory, ec);
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}
