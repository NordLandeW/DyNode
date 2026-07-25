// Lua-facing integration functions provided by DyCore.
//
// Functions declared here are registered under the `dynode` namespace and
// form the bridge between Lua scripts and the running GameMaker application.

#pragma once

#include <string>

#include "luaext.h"

// Queue an announcement for the GameMaker UI.
//
// `type` is case-insensitive and accepts "info"/"normal",
// "warning"/"warn", or "error". `lastTime` is the display duration in
// milliseconds and must not be negative.
void ll_gm_announcement(std::string str, std::string type, int lastTime);

// Register all DyCore-provided libraries in a Lua state.
void game_lualayer_openlibs(lua_State *);
