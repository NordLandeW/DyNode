/// @description Execute a function exposed to Lua's GM_EXECUTE event.
/// @param {String} function_name Whitelisted GameMaker function name.
/// @param {Array<Any>} args Function arguments.
/// @returns {Any} The called function's return value.
function gm_execute(function_name, args) {
	if(!is_string(function_name)) {
		throw "gm_execute function_name must be a string.";
	}
	if(!is_array(args)) {
		throw "gm_execute args must be an array.";
	}

	/// @self Function
	var _function = variable_global_get(function_name);
	return script_execute_ext(_function, args);
}

