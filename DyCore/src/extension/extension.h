#pragma once

#include <filesystem>

// Run the specified Lua script in a fresh Lua state.
void run_lua_script(const std::filesystem::path& scriptPath);

// Run script.lua from the process working directory.
void run_lua_script();
