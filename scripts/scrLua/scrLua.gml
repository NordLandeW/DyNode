function lua_run(luaPath = "script.lua") {
    try {
        var _response = json_parse(DyCore_lua_start(luaPath));

        while(_response[$ "state"] == "suspended") {
            if(_response[$ "resultType"] != "event") {
                throw "Lua coroutine returned an unsupported suspended result.";
            }

            var _event = _response[$ "result"];
            var _resultStr;
            try {
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
            } catch(e) {
                _resultStr = json_stringify({ error: string(e) });
            }

            _response = json_parse(
                DyCore_lua_resume(_resultStr)
            );
        }

        if(_response[$ "state"] != "dead") {
            throw "Lua coroutine returned an unknown state.";
        }
        if(variable_struct_exists(_response, "error")) {
            throw _response[$ "error"];
        }
        if(variable_struct_exists(_response, "result")) {
            return _response[$ "result"];
        }
        return undefined;
    } catch(e) {
        DyCore_lua_cancel();
        announcement_error($"Lua run failed.\nDetails:\n[scale,0.6]{e}");
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
        var _result = lua_run(_luaPath);
        if(file_exists(_luaPath)) {
            file_delete(_luaPath);
        }
        return _result;
    } catch(e) {
        if(file_exists(_luaPath)) {
            file_delete(_luaPath);
        }
        throw e;
    }
}
