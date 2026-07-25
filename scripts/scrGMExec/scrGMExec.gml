/// @description Execute a function exposed to DyCore's GM_EXECUTE event.
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

	var _function;
	switch(function_name) {
		case "editor_set_editmode": _function = editor_set_editmode; break;
		case "editor_get_editmode": _function = editor_get_editmode; break;
		case "editor_mode_is_switching": _function = editor_mode_is_switching; break;
		case "editor_get_default_width": _function = editor_get_default_width; break;
		case "editor_set_default_width_qbox": _function = editor_set_default_width_qbox; break;
		case "editor_set_default_width": _function = editor_set_default_width; break;
		case "editor_set_editside": _function = editor_set_editside; break;
		case "editor_get_editside": _function = editor_get_editside; break;
		case "editor_select_is_going": _function = editor_select_is_going; break;
		case "editor_select_is_multiple": _function = editor_select_is_multiple; break;
		case "editor_select_is_dragging": _function = editor_select_is_dragging; break;
		case "editor_select_is_area": _function = editor_select_is_area; break;
		case "editor_editside_allowed": _function = editor_editside_allowed; break;
		case "editor_lrside_set": _function = editor_lrside_set; break;
		case "editor_lrside_get": _function = editor_lrside_get; break;
		case "editor_lrside_lock_set": _function = editor_lrside_lock_set; break;
		case "editor_select_get_area_position": _function = editor_select_get_area_position; break;
		case "editor_select_inbound": _function = editor_select_inbound; break;
		case "editor_select_count": _function = editor_select_count; break;
		case "editor_get_selected_notes": _function = editor_get_selected_notes; break;
		case "editor_select_reset": _function = editor_select_reset; break;
		case "editor_select_all": _function = editor_select_all; break;
		case "editor_snap_to_grid_time": _function = editor_snap_to_grid_time; break;
		case "editor_snap_to_grid_y": _function = editor_snap_to_grid_y; break;
		case "editor_snap_to_grid_x": _function = editor_snap_to_grid_x; break;
		case "editor_snap_width": _function = editor_snap_width; break;
		case "editor_select_compare": _function = editor_select_compare; break;
		case "note_build_attach": _function = note_build_attach; break;
		case "editor_get_note_attaching_center": _function = editor_get_note_attaching_center; break;
		case "editor_note_duplicate_quick": _function = editor_note_duplicate_quick; break;
		case "editor_deduplicate_notes": _function = editor_deduplicate_notes; break;
		case "operation_get_name": _function = operation_get_name; break;
		case "operation_synctime_set": _function = operation_synctime_set; break;
		case "operation_synctime_sync": _function = operation_synctime_sync; break;
		case "operation_step_add": _function = operation_step_add; break;
		case "operation_step_flush": _function = operation_step_flush; break;
		case "operation_do": _function = operation_do; break;
		case "operation_undo": _function = operation_undo; break;
		case "operation_redo": _function = operation_redo; break;
		case "operation_merge_last": _function = operation_merge_last; break;
		case "operation_merge_last_request": _function = operation_merge_last_request; break;
		case "operation_merge_last_request_revoke": _function = operation_merge_last_request_revoke; break;
		case "timing_point_count": _function = timing_point_count; break;
		case "timing_point_sort": _function = timing_point_sort; break;
		case "timing_point_add": _function = timing_point_add; break;
		case "timing_point_create": _function = timing_point_create; break;
		case "timing_point_change": _function = timing_point_change; break;
		case "timing_fix": _function = timing_fix; break;
		case "timing_point_get_at": _function = timing_point_get_at; break;
		case "timing_point_delete_at": _function = timing_point_delete_at; break;
		case "timing_point_duplicate": _function = timing_point_duplicate; break;
		case "timing_point_reset": _function = timing_point_reset; break;
		case "chart_randomize": _function = chart_randomize; break;
		case "advanced_expr": _function = advanced_expr; break;
		case "editor_set_div": _function = editor_set_div; break;
		case "editor_get_div": _function = editor_get_div; break;
		case "note_outbound_warning": _function = note_outbound_warning; break;
		case "note_cover_warning": _function = note_cover_warning; break;
		case "editor_linear_sampling": _function = editor_linear_sampling; break;
		case "editor_cosine_sampling": _function = editor_cosine_sampling; break;
		case "editor_catmull_rom_sampling": _function = editor_catmull_rom_sampling; break;
		case "editor_cubic_sampling": _function = editor_cubic_sampling; break;
		case "editor_sampling_sametime_check": _function = editor_sampling_sametime_check; break;
		case "editor_selected_centrialize": _function = editor_selected_centrialize; break;
		case "editor_fix_notes": _function = editor_fix_notes; break;
		default:
			throw $"Unsupported GameMaker function for GM_EXECUTE: {function_name}";
	}

	return script_execute_ext(_function, args);
}

