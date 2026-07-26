
function LuaResult() constructor {
    state = "suspended"
    resultType = ""
    /// @type {Any}
    result = undefined
    error = ""
}

/// @returns {Struct.LuaResult}
function lua_run(luaPath = "script.lua") {
    try {
        /// @type {Struct.LuaResult}
        var _response = json_parse(DyCore_lua_start(luaPath));

        while(_response.state == "suspended") {
            if(_response.resultType != "event") {
                throw "Lua coroutine returned an unsupported suspended result.";
            }

            var _event = _response.result;
            var _resultStr;
            switch(_event[$ "type"]) {
                case "GM_EXECUTE":
                    var _result = gm_execute(
                        _event[$ "name"],
                        _event[$ "args"]
                    );
                    _resultStr = is_undefined(_result)
                        ? json_stringify({})
                        : json_stringify({ result: _result });
                    break;
                default:
                    throw $"Unsupported Lua event: {_event[$ "type"]}";
            }

            _response = json_parse(
                DyCore_lua_resume(_resultStr)
            );
        }

        if(_response.state == "error") {
            throw _response[$ "error"];
        }
        if(_response.state != "dead") {
            throw "Lua coroutine returned an unknown state.";
        }
        return _response;
    } catch(e) {
        DyCore_lua_cancel();
        var _response = new LuaResult();
        _response.state = "error";
        _response.error = string(e);
        return _response;
    }
}

function lua_run_script(luaScript) {
    static _scriptIndex = 0;
    _scriptIndex++;

    var _luaPath = temp_directory
        + "dynode_lua_run_script_"
        + string(get_timer())
        + "_"
        + string(_scriptIndex)
        + ".lua";

    try {
        SnapStringToFile(luaScript, _luaPath);
        var _response = lua_run(_luaPath);
        if(file_exists(_luaPath)) {
            file_delete(_luaPath);
        }
        return _response;
    } catch(e) {
        if(file_exists(_luaPath)) {
            file_delete(_luaPath);
        }
        return {
            state: "error",
            error: string(e)
        };
    }
}
