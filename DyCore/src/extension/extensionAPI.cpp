#include <exception>
#include <string>

#include "api.h"
#include "extension.h"
#include "utils.h"

namespace {

std::string luaRunnerResult;

const char* store_lua_result(nlohmann::json result) {
    luaRunnerResult =
        result.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    return luaRunnerResult.c_str();
}

}  // namespace

DYCORE_API const char* DyCore_lua_start(const char* luaPath) {
    try {
        if (luaPath == nullptr || *luaPath == '\0') {
            return store_lua_result(
                lua_error_result("luaPath cannot be empty."));
        }
        return store_lua_result(run_lua_script(convert_char_to_path(luaPath)));
    } catch (const std::exception& e) {
        return store_lua_result(lua_error_result(e.what()));
    } catch (...) {
        return store_lua_result(
            lua_error_result("Failed to start Lua coroutine: unknown error."));
    }
}

DYCORE_API const char* DyCore_lua_resume(const char* result) {
    try {
        if (result == nullptr) {
            return store_lua_result(
                lua_error_result("GameMaker result cannot be null."));
        }

        return store_lua_result(
            resume_lua_script(nlohmann::json::parse(result)));
    } catch (const std::exception& e) {
        cancel_lua_script();
        return store_lua_result(lua_error_result(e.what()));
    } catch (...) {
        cancel_lua_script();
        return store_lua_result(
            lua_error_result("Failed to resume Lua coroutine: unknown error."));
    }
}

DYCORE_API double DyCore_lua_cancel() {
    cancel_lua_script();
    return 0.0;
}
