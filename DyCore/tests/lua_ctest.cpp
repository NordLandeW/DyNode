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
#include "luaRunner.h"
#include "luaext.h"
#include "lualib.h"

extern "C" double DyCore_has_async_event();
extern "C" const char* DyCore_get_async_event();
extern "C" const char* DyCore_lua_start(const char* luaPath);
extern "C" const char* DyCore_lua_resume(const char* result);
extern "C" double DyCore_lua_cancel();

namespace {

std::filesystem::path make_lua_temp_dir() {
    const auto ticks =
        std::chrono::steady_clock::now().time_since_epoch().count();
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
    namespace fs = std::filesystem;

    while (DyCore_has_async_event() > 0) {
        DyCore_get_async_event();
    }

    const fs::path dir = make_lua_temp_dir();
    const fs::path scriptPath = dir / "announcement.lua";
    {
        std::ofstream script(scriptPath, std::ios::binary | std::ios::trunc);
        REQUIRE(script.is_open());
        script << "dynode.gm.announce("
                  "'Lua announcement', 'warning', 4321)\n";
    }

    try {
        LuaRunner runner(scriptPath);
        const auto result = runner.start();
        CHECK(result.at("state") == "dead");
        CHECK_FALSE(result.contains("error"));
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    REQUIRE(DyCore_has_async_event() > 0);

    const auto event = nlohmann::json::parse(DyCore_get_async_event());
    const auto content = nlohmann::json::parse(
        event.at("content").get_ref<const std::string&>());

    CHECK(event.at("status") ==
          static_cast<int>(GM_ANNOUNCEMENT_TYPE::ANNO_WARNING));
    CHECK(content.at("msg") == "Lua announcement");
    CHECK(content.at("args").empty());
    CHECK(content.at("lastTime") == 4321);

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaGmDirectoryPropertiesReflectProcessDirectories") {
    namespace fs = std::filesystem;

    const fs::path originalWorkingDirectory = fs::current_path();
    const fs::path dir = make_lua_temp_dir();

    try {
        fs::current_path(dir);

        const std::string programDirectory = ll_gm_prop_get_program_directory();
        const std::string workingDirectory = ll_gm_prop_get_working_directory();

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

        const fs::path scriptPath = dir / "directories.lua";
        {
            std::ofstream script(scriptPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "return {\n"
                   << "    program = dynode.ProgramDirectory,\n"
                   << "    working = dynode.WorkingDirectory,\n"
                   << "    version = dynode.Version\n"
                   << "}\n";
        }

        LuaRunner runner(scriptPath);
        const auto result = runner.start();
        REQUIRE(result.at("state") == "dead");
        REQUIRE(result.at("resultType") == "struct");
        CHECK(result.at("result").at("program") == programDirectory);
        CHECK(result.at("result").at("working") == workingDirectory);
        CHECK(result.at("result").at("version") == ll_prop_get_game_version());

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
    namespace fs = std::filesystem;

    gmeditor_sync_states({{"editMode", 4}});

    const fs::path dir = make_lua_temp_dir();
    const fs::path scriptPath = dir / "editor_getter.lua";
    {
        std::ofstream script(scriptPath, std::ios::binary | std::ios::trunc);
        REQUIRE(script.is_open());
        script << "return dynode.editor.get_editmode()\n";
    }

    try {
        LuaRunner runner(scriptPath);
        const auto result = runner.start();
        CHECK(result.at("state") == "dead");
        CHECK(result.at("resultType") == "double");
        CHECK(result.at("result") == 4.0);
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaRunnerReturnsArray") {
    namespace fs = std::filesystem;

    const fs::path dir = make_lua_temp_dir();
    const fs::path scriptPath = dir / "array.lua";
    {
        std::ofstream script(scriptPath, std::ios::binary | std::ios::trunc);
        REQUIRE(script.is_open());
        script << "return {1, 'two', {three = 3}}\n";
    }

    try {
        LuaRunner runner(scriptPath);
        const auto result = runner.start();

        CHECK(result.at("state") == "dead");
        CHECK(result.at("resultType") == "array");
        CHECK(result.at("result") ==
              nlohmann::json::array(
                  {1.0, "two", nlohmann::json{{"three", 3.0}}}));
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaRunnerResumesGameMakerExecuteWithResult") {
    namespace fs = std::filesystem;

    const fs::path dir = make_lua_temp_dir();
    const fs::path scriptPath = dir / "gamemaker_execute.lua";
    {
        std::ofstream script(scriptPath, std::ios::binary | std::ios::trunc);
        REQUIRE(script.is_open());
        script << "local result = dynode.editor.set_editmode(1)\n"
               << "return {\n"
               << "    editMode = dynode.editor.get_editmode(),\n"
               << "    result = result\n"
               << "}\n";
    }

    try {
        LuaRunner runner(scriptPath);
        const auto suspended = runner.start();

        REQUIRE(suspended.at("state") == "suspended");
        CHECK(suspended.at("resultType") == "event");
        const auto& event = suspended.at("result");
        CHECK(event.at("type") == "GM_EXECUTE");
        CHECK(event.at("name") == "editor_set_editmode");
        CHECK(event.at("args") == nlohmann::json::array({1}));

        gmeditor_sync_states({{"editMode", 1}});
        const auto completed =
            runner.resume({{"result", nlohmann::json::array()}});

        CHECK(completed.at("state") == "dead");
        CHECK(completed.at("resultType") == "struct");
        CHECK(completed.at("result").at("editMode") == 1.0);
        CHECK(completed.at("result").at("result") == nlohmann::json::array());
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaRunnerExecutesArbitraryGameMakerFunctionsWithVarargs") {
    namespace fs = std::filesystem;

    const fs::path dir = make_lua_temp_dir();
    const fs::path scriptPath = dir / "gamemaker_exec.lua";
    {
        std::ofstream script(scriptPath, std::ios::binary | std::ios::trunc);
        REQUIRE(script.is_open());
        script << "local first = dynode.gm.exec(\n"
               << "    '__test_lua_gm_deep_io',\n"
               << "    {number = 1.25, nested = {true, nil, 'tail'}},\n"
               << "    nil,\n"
               << "    false,\n"
               << "    'text'\n"
               << ")\n"
               << "local second = dynode.gm.exec('__test_lua_gm_no_args')\n"
               << "return {first = first, second = second}\n";
    }

    try {
        LuaRunner runner(scriptPath);
        const auto firstEvent = runner.start();

        REQUIRE(firstEvent.at("state") == "suspended");
        CHECK(firstEvent.at("resultType") == "event");
        CHECK(firstEvent.at("result").at("name") == "__test_lua_gm_deep_io");
        CHECK(firstEvent.at("result").at("args") ==
              nlohmann::json::array(
                  {{{"number", 1.25},
                    {"nested", nlohmann::json::array({true, nullptr, "tail"})}},
                   nullptr,
                   false,
                   "text"}));

        const nlohmann::json firstResult = {
            {"fromGml",
             nlohmann::json::array(
                 {nullptr, true, {{"nested", nlohmann::json::array({1, 2})}}})},
        };
        const auto secondEvent = runner.resume({{"result", firstResult}});

        REQUIRE(secondEvent.at("state") == "suspended");
        CHECK(secondEvent.at("result").at("name") == "__test_lua_gm_no_args");
        CHECK(secondEvent.at("result").at("args") == nlohmann::json::array());

        const nlohmann::json secondResult = nlohmann::json::array();
        const auto completed = runner.resume({{"result", secondResult}});

        CHECK(completed.at("state") == "dead");
        CHECK(completed.at("resultType") == "struct");
        CHECK(completed.at("result") == nlohmann::json{
                                            {"first", firstResult},
                                            {"second", secondResult},
                                        });
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaRunnersKeepGameMakerExecutionStateIndependent") {
    namespace fs = std::filesystem;

    const fs::path dir = make_lua_temp_dir();
    const fs::path firstPath = dir / "first.lua";
    const fs::path secondPath = dir / "second.lua";
    {
        std::ofstream first(firstPath, std::ios::binary | std::ios::trunc);
        REQUIRE(first.is_open());
        first << "return dynode.gm.exec('first_runner')\n";

        std::ofstream second(secondPath, std::ios::binary | std::ios::trunc);
        REQUIRE(second.is_open());
        second << "return dynode.gm.exec('second_runner')\n";
    }

    try {
        LuaRunner firstRunner(firstPath);
        LuaRunner secondRunner(secondPath);

        const auto firstEvent = firstRunner.start();
        const auto secondEvent = secondRunner.start();

        REQUIRE(firstEvent.at("state") == "suspended");
        REQUIRE(secondEvent.at("state") == "suspended");
        CHECK(firstEvent.at("result").at("name") == "first_runner");
        CHECK(secondEvent.at("result").at("name") == "second_runner");

        const auto firstResult =
            firstRunner.resume({{"result", "first_result"}});
        const auto secondResult =
            secondRunner.resume({{"result", "second_result"}});

        CHECK(firstResult.at("state") == "dead");
        CHECK(firstResult.at("result") == "first_result");
        CHECK(secondResult.at("state") == "dead");
        CHECK(secondResult.at("result") == "second_result");
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaRunnerReturnsGameMakerErrorsToLua") {
    namespace fs = std::filesystem;

    const fs::path dir = make_lua_temp_dir();
    const fs::path scriptPath = dir / "gamemaker_error.lua";
    {
        std::ofstream script(scriptPath, std::ios::binary | std::ios::trunc);
        REQUIRE(script.is_open());
        script << "local ok, err = pcall(function()\n"
               << "    dynode.editor.set_editmode(1)\n"
               << "end)\n"
               << "return {ok = ok, error = err}\n";
    }

    try {
        LuaRunner runner(scriptPath);
        const auto suspended = runner.start();
        REQUIRE(suspended.at("state") == "suspended");

        const auto completed =
            runner.resume({{"error", "intentional GameMaker failure"}});

        CHECK(completed.at("state") == "dead");
        CHECK(completed.at("resultType") == "struct");
        CHECK(completed.at("result").at("ok") == false);
        CHECK(completed.at("result").at("error").get<std::string>().find(
                  "intentional GameMaker failure") != std::string::npos);
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaApiProcessesGameMakerExecuteWithoutAsyncEvents") {
    namespace fs = std::filesystem;

    while (DyCore_has_async_event() > 0) {
        DyCore_get_async_event();
    }
    DyCore_lua_cancel();

    const fs::path dir = make_lua_temp_dir();
    const fs::path scriptPath = dir / "lua_api.lua";
    {
        std::ofstream script(scriptPath, std::ios::binary | std::ios::trunc);
        REQUIRE(script.is_open());
        script << "dynode.editor.set_editmode(1)\n"
               << "dynode.editor.set_editmode(5)\n"
               << "return 'done'\n";
    }

    try {
        auto response = nlohmann::json::parse(
            DyCore_lua_start(scriptPath.string().c_str()));
        REQUIRE(response.at("state") == "suspended");
        CHECK(response.at("result").at("args") == nlohmann::json::array({1}));
        CHECK(DyCore_has_async_event() == 0);

        response = nlohmann::json::parse(DyCore_lua_resume("{}"));
        REQUIRE(response.at("state") == "suspended");
        CHECK(response.at("result").at("args") == nlohmann::json::array({5}));
        CHECK(DyCore_has_async_event() == 0);

        response = nlohmann::json::parse(DyCore_lua_resume("{}"));
        CHECK(response.at("state") == "dead");
        CHECK(response.at("resultType") == "string");
        CHECK(response.at("result") == "done");
        CHECK(DyCore_has_async_event() == 0);
    } catch (...) {
        DyCore_lua_cancel();
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaApiUsesErrorStateForInvalidRequests") {
    DyCore_lua_cancel();

    const auto invalidPath =
        nlohmann::json::parse(DyCore_lua_start(""));
    CHECK(invalidPath.at("state") == "error");
    CHECK(invalidPath.at("error") == "luaPath cannot be empty.");

    const auto invalidResult =
        nlohmann::json::parse(DyCore_lua_resume("{"));
    CHECK(invalidResult.at("state") == "error");
    CHECK(invalidResult.at("error").is_string());
}

TEST_CASE("LuaRunnerReportsCoroutineAndValueErrors") {
    namespace fs = std::filesystem;

    const fs::path dir = make_lua_temp_dir();
    try {
        const fs::path yieldPath = dir / "unsupported_yield.lua";
        {
            std::ofstream script(yieldPath, std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "coroutine.yield('unsupported')\n";
        }
        LuaRunner yieldRunner(yieldPath);
        const auto yieldResult = yieldRunner.start();
        CHECK(yieldResult.at("state") == "error");
        CHECK(yieldResult.at("error").get<std::string>().find("unsupported") !=
              std::string::npos);

        const fs::path cyclicPath = dir / "cyclic_result.lua";
        {
            std::ofstream script(cyclicPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "local result = {}\n"
                   << "result.self = result\n"
                   << "return result\n";
        }
        LuaRunner cyclicRunner(cyclicPath);
        const auto cyclicResult = cyclicRunner.start();
        CHECK(cyclicResult.at("state") == "error");
        CHECK(cyclicResult.at("error").get<std::string>().find("cyclic") !=
              std::string::npos);

        const fs::path runtimeErrorPath = dir / "runtime_error.lua";
        {
            std::ofstream script(runtimeErrorPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "local function fail()\n"
                   << "    error('intentional Lua runtime error')\n"
                   << "end\n"
                   << "fail()\n";
        }
        LuaRunner runtimeErrorRunner(runtimeErrorPath);
        const auto runtimeErrorResult = runtimeErrorRunner.start();
        CHECK(runtimeErrorResult.at("state") == "error");
        const auto error = runtimeErrorResult.at("error").get<std::string>();
        CHECK(error.find("intentional Lua runtime error") != std::string::npos);
        CHECK(error.find("stack traceback") != std::string::npos);
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("LuaRunnerDeepConvertsJSONValues") {
    namespace fs = std::filesystem;

    const fs::path dir = make_lua_temp_dir();
    try {
        const fs::path argumentPath = dir / "invalid_argument.lua";
        {
            std::ofstream script(argumentPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "return dynode.editor.set_editmode(true)\n";
        }
        LuaRunner argumentRunner(argumentPath);
        const auto argumentResult = argumentRunner.start();
        CHECK(argumentResult.at("state") == "error");
        CHECK(argumentResult.at("error").get<std::string>().find(
                  "must be a double") != std::string::npos);

        const fs::path nonPositiveArgumentPath =
            dir / "non_positive_argument.lua";
        {
            std::ofstream script(nonPositiveArgumentPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "return dynode.editor.set_editmode(0)\n";
        }
        LuaRunner nonPositiveArgumentRunner(nonPositiveArgumentPath);
        const auto nonPositiveArgumentResult =
            nonPositiveArgumentRunner.start();
        CHECK(nonPositiveArgumentResult.at("state") == "error");
        CHECK(nonPositiveArgumentResult.at("error").get<std::string>().find(
                  "non-positive") != std::string::npos);

        const fs::path nestedBooleanPath = dir / "nested_boolean.lua";
        {
            std::ofstream script(nestedBooleanPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "return {nested = {true}}\n";
        }
        LuaRunner nestedBooleanRunner(nestedBooleanPath);
        const auto nestedBooleanResult = nestedBooleanRunner.start();
        CHECK(nestedBooleanResult.at("state") == "dead");
        CHECK(nestedBooleanResult.at("resultType") == "struct");
        CHECK(nestedBooleanResult.at("result") ==
              nlohmann::json{{"nested", nlohmann::json::array({true})}});

        const fs::path resultPath = dir / "deep_gamemaker_result.lua";
        {
            std::ofstream script(resultPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "return dynode.editor.set_editmode(1)\n";
        }
        LuaRunner resultRunner(resultPath);
        const auto suspended = resultRunner.start();
        REQUIRE(suspended.at("state") == "suspended");
        const nlohmann::json deepResult =
            nlohmann::json::array({true, nullptr, "tail"});
        const auto resumedResult =
            resultRunner.resume({{"result", deepResult}});
        CHECK(resumedResult.at("state") == "dead");
        CHECK(resumedResult.at("resultType") == "array");
        CHECK(resumedResult.at("result") == deepResult);

        const fs::path noResultPath = dir / "no_result.lua";
        {
            std::ofstream script(noResultPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "return\n";
        }
        LuaRunner noResultRunner(noResultPath);
        const auto noResult = noResultRunner.start();
        CHECK(noResult.at("state") == "dead");
        CHECK(noResult.at("resultType") == "undefined");
        CHECK(noResult.at("result").is_null());

        const fs::path sparseArrayPath = dir / "sparse_array.lua";
        {
            std::ofstream script(sparseArrayPath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "return {[1] = true, [3] = 'tail'}\n";
        }
        LuaRunner sparseArrayRunner(sparseArrayPath);
        const auto sparseArray = sparseArrayRunner.start();
        CHECK(sparseArray.at("state") == "dead");
        CHECK(sparseArray.at("resultType") == "array");
        CHECK(sparseArray.at("result") ==
              nlohmann::json::array({true, nullptr, "tail"}));

        const fs::path mixedTablePath = dir / "mixed_table.lua";
        {
            std::ofstream script(mixedTablePath,
                                 std::ios::binary | std::ios::trunc);
            REQUIRE(script.is_open());
            script << "return {[1] = 'array', named = 'struct'}\n";
        }
        LuaRunner mixedTableRunner(mixedTablePath);
        const auto mixedTable = mixedTableRunner.start();
        CHECK(mixedTable.at("state") == "error");
        CHECK(mixedTable.at("error").get<std::string>().find(
                  "arrays or structs") != std::string::npos);
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        throw;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
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
        const auto completed = run_lua_script("script.lua");
        CHECK(completed.at("state") == "dead");
        CHECK_FALSE(completed.contains("error"));

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
        const auto failed = run_lua_script("script.lua");
        CHECK(failed.at("state") == "error");
        CHECK(failed.at("error").get<std::string>().find(
                  "intentional Lua failure") != std::string::npos);

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
