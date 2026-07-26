#include "luaRunner.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "lualib.h"

namespace {

constexpr int maxLuaValueDepth = 64;
constexpr const char* luaArrayTypeField = "__dycoreType";
constexpr const char* luaArrayLengthField = "__dycoreLength";

std::optional<lua_Integer> marked_array_length(lua_State* lua, int index) {
    if (lua_getmetatable(lua, index) == 0) {
        return std::nullopt;
    }

    lua_getfield(lua, -1, luaArrayTypeField);
    const bool isArray = lua_isstring(lua, -1) &&
                         std::string_view(lua_tostring(lua, -1)) == "array";
    lua_pop(lua, 1);
    if (!isArray) {
        lua_pop(lua, 1);
        return std::nullopt;
    }

    lua_getfield(lua, -1, luaArrayLengthField);
    if (!lua_isinteger(lua, -1) || lua_tointeger(lua, -1) < 0) {
        lua_pop(lua, 2);
        throw std::runtime_error("Lua array metadata is invalid.");
    }
    const lua_Integer length = lua_tointeger(lua, -1);
    lua_pop(lua, 2);
    return length;
}

void mark_as_array(lua_State* lua, int index, lua_Integer length) {
    lua_createtable(lua, 0, 2);
    lua_pushliteral(lua, "array");
    lua_setfield(lua, -2, luaArrayTypeField);
    lua_pushinteger(lua, length);
    lua_setfield(lua, -2, luaArrayLengthField);
    lua_setmetatable(lua, index);
}

nlohmann::json lua_value_to_json(lua_State* lua, int index,
                                 std::unordered_set<const void*>& activeTables,
                                 int depth);

class ActiveLuaTable {
   public:
    ActiveLuaTable(std::unordered_set<const void*>& activeTables,
                   const void* tablePointer)
        : activeTables(activeTables), tablePointer(tablePointer) {
        if (!activeTables.insert(tablePointer).second) {
            throw std::runtime_error("Lua result contains a cyclic table.");
        }
    }

    ~ActiveLuaTable() {
        activeTables.erase(tablePointer);
    }

    ActiveLuaTable(const ActiveLuaTable&) = delete;
    ActiveLuaTable& operator=(const ActiveLuaTable&) = delete;

   private:
    std::unordered_set<const void*>& activeTables;
    const void* tablePointer;
};

struct LuaTableShape {
    lua_Integer arrayLength;
    bool isArray;
    bool isStruct;
};

LuaTableShape inspect_lua_table(lua_State* lua, int tableIndex) {
    const auto markedLength = marked_array_length(lua, tableIndex);
    LuaTableShape shape{markedLength.value_or(0), true, true};
    bool hasArrayKey = false;

    lua_pushnil(lua);
    while (lua_next(lua, tableIndex) != 0) {
        if (!lua_isinteger(lua, -2)) {
            shape.isArray = false;
        } else {
            const lua_Integer key = lua_tointeger(lua, -2);
            if (key < 1) {
                shape.isArray = false;
            } else {
                hasArrayKey = true;
                shape.arrayLength = std::max(shape.arrayLength, key);
            }
        }

        if (lua_type(lua, -2) != LUA_TSTRING) {
            shape.isStruct = false;
        }

        lua_pop(lua, 1);
    }

    if (!hasArrayKey && !markedLength.has_value()) {
        shape.isArray = false;
    }
    return shape;
}

nlohmann::json lua_array_to_json(lua_State* lua, int tableIndex,
                                 lua_Integer arrayLength,
                                 std::unordered_set<const void*>& activeTables,
                                 int depth) {
    nlohmann::json result = nlohmann::json::array();
    for (lua_Integer i = 1; i <= arrayLength; ++i) {
        lua_rawgeti(lua, tableIndex, i);
        result.push_back(lua_value_to_json(lua, -1, activeTables, depth + 1));
        lua_pop(lua, 1);
    }
    return result;
}

nlohmann::json lua_struct_to_json(lua_State* lua, int tableIndex,
                                  std::unordered_set<const void*>& activeTables,
                                  int depth) {
    nlohmann::json result = nlohmann::json::object();
    lua_pushnil(lua);
    while (lua_next(lua, tableIndex) != 0) {
        const char* key = lua_tostring(lua, -2);
        result[key] = lua_value_to_json(lua, -1, activeTables, depth + 1);
        lua_pop(lua, 1);
    }
    return result;
}

nlohmann::json lua_table_to_json(lua_State* lua, int index,
                                 std::unordered_set<const void*>& activeTables,
                                 int depth) {
    if (depth > maxLuaValueDepth) {
        throw std::runtime_error("Lua result exceeds the maximum table depth.");
    }

    const int tableIndex = lua_absindex(lua, index);
    const void* tablePointer = lua_topointer(lua, tableIndex);
    const ActiveLuaTable activeTable(activeTables, tablePointer);
    const LuaTableShape shape = inspect_lua_table(lua, tableIndex);

    if (shape.isArray) {
        return lua_array_to_json(lua, tableIndex, shape.arrayLength,
                                 activeTables, depth);
    }
    if (shape.isStruct) {
        return lua_struct_to_json(lua, tableIndex, activeTables, depth);
    }
    throw std::runtime_error(
        "Lua result tables must be arrays or structs with string keys.");
}

nlohmann::json lua_value_to_json(lua_State* lua, int index,
                                 std::unordered_set<const void*>& activeTables,
                                 int depth) {
    switch (lua_type(lua, index)) {
        case LUA_TNIL:
            return nullptr;
        case LUA_TBOOLEAN:
            return lua_toboolean(lua, index) != 0;
        case LUA_TNUMBER: {
            const lua_Number value = lua_tonumber(lua, index);
            if (!std::isfinite(value)) {
                throw std::runtime_error("Lua result numbers must be finite.");
            }
            return static_cast<double>(value);
        }
        case LUA_TSTRING: {
            size_t length = 0;
            const char* value = lua_tolstring(lua, index, &length);
            return std::string(value, length);
        }
        case LUA_TTABLE:
            return lua_table_to_json(lua, index, activeTables, depth);
        default:
            throw std::runtime_error(
                std::string("Unsupported Lua result type: ") +
                lua_typename(lua, lua_type(lua, index)));
    }
}

nlohmann::json lua_value_to_json(lua_State* lua, int index) {
    std::unordered_set<const void*> activeTables;
    return lua_value_to_json(lua, index, activeTables, 0);
}

void validate_gamemaker_value(const nlohmann::json& value, int depth = 0) {
    if (depth > maxLuaValueDepth) {
        throw std::runtime_error(
            "GameMaker values exceed the maximum value depth.");
    }

    if (value.is_null() || value.is_boolean()) {
        return;
    }
    if (value.is_number()) {
        const double number = value.get<double>();
        if (!std::isfinite(number)) {
            throw std::runtime_error(
                "GameMaker values must contain finite doubles.");
        }
        return;
    }
    if (value.is_string()) {
        return;
    }
    if (value.is_array()) {
        for (const auto& element : value) {
            validate_gamemaker_value(element, depth + 1);
        }
        return;
    }
    if (value.is_object()) {
        for (auto it = value.begin(); it != value.end(); ++it) {
            validate_gamemaker_value(it.value(), depth + 1);
        }
        return;
    }

    throw std::runtime_error("Unsupported GameMaker value type.");
}

void json_to_lua_value(lua_State* lua, const nlohmann::json& value,
                       int depth = 0) {
    if (depth > maxLuaValueDepth) {
        throw std::runtime_error(
            "GameMaker result exceeds the maximum value depth.");
    }

    if (value.is_null()) {
        lua_pushnil(lua);
    } else if (value.is_boolean()) {
        lua_pushboolean(lua, value.get<bool>());
    } else if (value.is_number()) {
        const double number = value.get<double>();
        if (!std::isfinite(number)) {
            throw std::runtime_error(
                "GameMaker result numbers must be finite.");
        }
        lua_pushnumber(lua, number);
    } else if (value.is_string()) {
        const auto& string = value.get_ref<const std::string&>();
        lua_pushlstring(lua, string.data(), string.size());
    } else if (value.is_array()) {
        lua_createtable(lua, static_cast<int>(value.size()), 0);
        const int tableIndex = lua_gettop(lua);
        mark_as_array(lua, tableIndex, static_cast<lua_Integer>(value.size()));
        for (size_t i = 0; i < value.size(); ++i) {
            json_to_lua_value(lua, value[i], depth + 1);
            lua_rawseti(lua, tableIndex, static_cast<lua_Integer>(i + 1));
        }
    } else if (value.is_object()) {
        lua_createtable(lua, 0, static_cast<int>(value.size()));
        const int tableIndex = lua_gettop(lua);
        for (auto it = value.begin(); it != value.end(); ++it) {
            json_to_lua_value(lua, it.value(), depth + 1);
            lua_setfield(lua, tableIndex, it.key().c_str());
        }
    } else {
        throw std::runtime_error("Unsupported GameMaker result type.");
    }
}

std::string result_type(const nlohmann::json& result) {
    if (result.is_null()) {
        return "undefined";
    }
    if (result.is_boolean()) {
        return "boolean";
    }
    if (result.is_number()) {
        return "double";
    }
    if (result.is_string()) {
        return "string";
    }
    if (result.is_array()) {
        return "array";
    }
    if (result.is_object()) {
        return "struct";
    }
    throw std::runtime_error("Unsupported Lua result type.");
}

std::string lua_error_message(lua_State* lua) {
    const char* message = lua_tostring(lua, -1);
    const std::string error =
        message != nullptr ? message : "unknown Lua error";

    luaL_traceback(lua, lua, error.c_str(), 1);
    const char* traceback = lua_tostring(lua, -1);
    const std::string result = traceback != nullptr ? traceback : error;
    lua_pop(lua, 1);
    return result;
}

}  // namespace

void LuaRunner::LuaStateDeleter::operator()(lua_State* lua) const {
    if (lua != nullptr) {
        lua_close(lua);
    }
}

LuaRunner::LuaRunner(const std::filesystem::path& luaPath)
    : lua(luaL_newstate()) {
    if (!lua) {
        throw std::runtime_error("Failed to create Lua state.");
    }

    luaL_openlibs(lua.get());
    game_lualayer_openlibs(*this);

    luaCoroutine = lua_newthread(lua.get());
    if (luaCoroutine == nullptr) {
        throw std::runtime_error("Failed to create Lua coroutine.");
    }
    luaCoroutineRef = luaL_ref(lua.get(), LUA_REGISTRYINDEX);

    const std::string luaPathString = luaPath.string();
    if (luaL_loadfile(luaCoroutine, luaPathString.c_str()) != LUA_OK) {
        throw std::runtime_error("Failed to load Lua script '" + luaPathString +
                                 "': " + lua_error_message(luaCoroutine));
    }
}

LuaRunner::~LuaRunner() {
    if (lua && luaCoroutineRef != LUA_NOREF) {
        luaL_unref(lua.get(), LUA_REGISTRYINDEX, luaCoroutineRef);
    }
}

nlohmann::json LuaRunner::start() {
    if (started) {
        return finish_with_error("Lua coroutine has already started.");
    }
    started = true;
    return run();
}

nlohmann::json LuaRunner::resume(nlohmann::json result) {
    if (!started) {
        return finish_with_error("Lua coroutine has not started.");
    }
    if (dead) {
        return finish_with_error("Lua coroutine is already dead.");
    }
    if (!waitingForGamemaker) {
        return finish_with_error(
            "Lua coroutine is not waiting for a GameMaker result.");
    }

    gamemakerResult = std::move(result);
    return run();
}

bool LuaRunner::is_dead() const {
    return dead;
}

luabridge::CppCoroutine<luabridge::LuaRef> LuaRunner::gamemaker_execute(
    std::string functionName, std::vector<luabridge::LuaRef> args) {
    if (waitingForGamemaker) {
        throw std::runtime_error(
            "Another GameMaker execute request is already pending.");
    }

    nlohmann::json convertedArgs = nlohmann::json::array();
    for (const auto& arg : args) {
        arg.push(luaCoroutine);
        try {
            convertedArgs.push_back(lua_value_to_json(luaCoroutine, -1));
            lua_pop(luaCoroutine, 1);
        } catch (...) {
            lua_pop(luaCoroutine, 1);
            throw;
        }
    }
    validate_gamemaker_value(convertedArgs);

    waitingForGamemaker = true;
    gamemakerResult.reset();

    nlohmann::json event = {
        {"type", "GM_EXECUTE"},
        {"name", std::move(functionName)},
        {"args", std::move(convertedArgs)},
    };
    co_yield luabridge::LuaRef(luaCoroutine, event.dump());

    if (!gamemakerResult.has_value()) {
        waitingForGamemaker = false;
        throw std::runtime_error(
            "GameMaker did not return a result for GM_EXECUTE.");
    }

    nlohmann::json result = std::move(*gamemakerResult);
    gamemakerResult.reset();
    waitingForGamemaker = false;

    if (!result.is_object()) {
        throw std::runtime_error(
            "GameMaker returned an invalid GM_EXECUTE result.");
    }
    if (result.contains("error")) {
        std::string error;
        if (result["error"].is_string()) {
            error = result["error"].get<std::string>();
        } else {
            error = result["error"].dump();
        }
        throw std::runtime_error("GM_EXECUTE failed: " + error);
    }

    if (!result.contains("result")) {
        co_return luabridge::LuaRef(luaCoroutine, luabridge::LuaNil{});
    }

    validate_gamemaker_value(result["result"]);
    json_to_lua_value(luaCoroutine, result["result"]);
    co_return luabridge::LuaRef::fromStack(luaCoroutine);
}

nlohmann::json LuaRunner::run() {
    int resultCount = 0;
    int status = LUA_OK;
    try {
        status = lua_resume(luaCoroutine, nullptr, 0, &resultCount);
    } catch (const std::exception& e) {
        return finish_with_error(e.what());
    } catch (...) {
        return finish_with_error(
            "Unknown C++ error while resuming the Lua coroutine.");
    }

    if (status == LUA_YIELD) {
        if (!waitingForGamemaker || resultCount != 1 ||
            !lua_isstring(luaCoroutine, -1)) {
            if (resultCount > 0) {
                lua_pop(luaCoroutine, resultCount);
            }
            return finish_with_error(
                "Lua coroutine yielded an unsupported value.");
        }

        size_t length = 0;
        const char* eventString = lua_tolstring(luaCoroutine, -1, &length);
        try {
            nlohmann::json event =
                nlohmann::json::parse(eventString, eventString + length);
            lua_pop(luaCoroutine, resultCount);
            return {
                {"state", "suspended"},
                {"resultType", "event"},
                {"result", std::move(event)},
            };
        } catch (const std::exception& e) {
            lua_pop(luaCoroutine, resultCount);
            return finish_with_error("Failed to parse the yielded Lua event: " +
                                     std::string(e.what()));
        }
    }

    if (status != LUA_OK) {
        return finish_with_error(lua_error_message(luaCoroutine));
    }

    if (resultCount > 1) {
        lua_pop(luaCoroutine, resultCount);
        return finish_with_error("Lua scripts must return at most one value.");
    }

    nlohmann::json result = nullptr;
    try {
        if (resultCount == 1) {
            result = lua_value_to_json(luaCoroutine, -1);
            lua_pop(luaCoroutine, 1);
        }
    } catch (const std::exception& e) {
        if (resultCount == 1) {
            lua_pop(luaCoroutine, 1);
        }
        return finish_with_error(e.what());
    }

    dead = true;
    return {
        {"state", "dead"},
        {"resultType", result_type(result)},
        {"result", std::move(result)},
    };
}

nlohmann::json LuaRunner::finish_with_error(std::string error) {
    dead = true;
    return {
        {"state", "error"},
        {"error", std::move(error)},
    };
}
