#include "extension.h"

#include <memory>
#include <mutex>
#include <stdexcept>

#include "luaRunner.h"

namespace {

std::unique_ptr<LuaRunner> activeLuaRunner;
std::mutex luaRunnerMutex;

void reset_dead_lua_runner() {
    if (activeLuaRunner != nullptr && activeLuaRunner->is_dead()) {
        activeLuaRunner.reset();
    }
}

}  // namespace

nlohmann::json run_lua_script(const std::filesystem::path& luaPath) {
    std::lock_guard<std::mutex> lock(luaRunnerMutex);

    if (activeLuaRunner != nullptr) {
        throw std::runtime_error("Another Lua coroutine is already running.");
    }

    std::filesystem::path path = luaPath;
    if (path.is_relative()) {
        path = std::filesystem::current_path() / path;
    }

    activeLuaRunner = std::make_unique<LuaRunner>(path);
    nlohmann::json result = activeLuaRunner->start();
    reset_dead_lua_runner();
    return result;
}

nlohmann::json resume_lua_script(nlohmann::json result) {
    std::lock_guard<std::mutex> lock(luaRunnerMutex);

    if (activeLuaRunner == nullptr) {
        throw std::runtime_error("No Lua coroutine is running.");
    }

    nlohmann::json response = activeLuaRunner->resume(std::move(result));
    reset_dead_lua_runner();
    return response;
}

void cancel_lua_script() {
    std::lock_guard<std::mutex> lock(luaRunnerMutex);
    activeLuaRunner.reset();
}

nlohmann::json lua_error_result(const std::string& error) {
    return {
        {"state", "dead"},
        {"error", error},
    };
}
