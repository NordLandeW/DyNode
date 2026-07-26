#pragma once

#include <filesystem>
#include <json.hpp>
#include <string>

// Start the specified Lua script in a new coroutine.
nlohmann::json run_lua_script(const std::filesystem::path& luaPath);

// Resume the active Lua coroutine with a GameMaker result.
nlohmann::json resume_lua_script(nlohmann::json result);

// Cancel the active Lua coroutine.
void cancel_lua_script();

// Create a terminal Lua error response.
nlohmann::json lua_error_result(const std::string& error);
