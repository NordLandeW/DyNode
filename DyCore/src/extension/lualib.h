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

// Return the absolute dynode.exe program directory.
std::string ll_gm_prop_get_program_directory();

// Return the absolute dynode.exe / DyCore / Lua script working directory.
std::string ll_gm_prop_get_working_directory();

std::string ll_prop_get_game_version();

int ll_editor_prop_get_editmode();

bool ll_editor_is_ready();

// Return the current GameMaker editor mode.
int ll_editor_prop_get_editmode();

// Register all DyCore-provided libraries in a Lua state.
void game_lualayer_openlibs(lua_State *);
