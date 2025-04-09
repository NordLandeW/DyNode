
#macro window_command_close    $F060
#macro window_command_maximize $F030
#macro window_command_minimize $F020
#macro window_command_restore  $F120
#macro window_command_resize   $F000
#macro window_command_move     $F010

function window_command_hook_prepare_buffer(_size) {
    gml_pragma("global", "global.__window_command_hook_buffer = undefined");
    var _buf = global.__window_command_hook_buffer;
    if (_buf == undefined) {
        _buf = buffer_create(_size, buffer_grow, 1);
        global.__window_command_hook_buffer = _buf;
    } else if (buffer_get_size(_buf) < _size) {
        buffer_resize(_buf, _size);
    }
    buffer_seek(_buf, buffer_seek_start, 0);
    return _buf;
}

function window_command_hook(command) {
    var _buf = window_command_hook_prepare_buffer(12);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_s32, command);
    return window_command_hook_raw(buffer_get_address(_buf), 12);
}

function window_command_unhook(command) {
    var _buf = window_command_hook_prepare_buffer(12);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_s32, command);
    return window_command_unhook_raw(buffer_get_address(_buf), 12);
}

function window_command_check(command) {
    var _buf = window_command_hook_prepare_buffer(4);
    buffer_write(_buf, buffer_s32, command);
    return window_command_check_raw(buffer_get_address(_buf), 4);
}

function window_command_run(wParam, lParam = 0) {
    var _buf = window_command_hook_prepare_buffer(17);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_s32, wParam);
    buffer_write(_buf, buffer_bool, argument_count >= 2);
    if (argument_count >= 2) {
        buffer_write(_buf, buffer_s32, lParam);
    }
    return window_command_run_raw(buffer_get_address(_buf), 17);
}

function window_command_get_active(command) {
    var _buf = window_command_hook_prepare_buffer(12);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_s32, command);
    return window_command_get_active_raw(buffer_get_address(_buf), 12);
}

function window_command_set_active(command, value) {
    var _buf = window_command_hook_prepare_buffer(13);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_s32, command);
    buffer_write(_buf, buffer_bool, value);
    return window_command_set_active_raw(buffer_get_address(_buf), 13);
}

function window_get_background_redraw() {
    var _buf = window_command_hook_prepare_buffer(1);
    return window_get_background_redraw_raw(buffer_get_address(_buf), 1);
}

function window_set_background_redraw(enable) {
    var _buf = window_command_hook_prepare_buffer(9);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_bool, enable);
    return window_set_background_redraw_raw(buffer_get_address(_buf), 9);
}

function window_get_topmost() {
    var _buf = window_command_hook_prepare_buffer(8);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    return window_get_topmost_raw(buffer_get_address(_buf), 8);
}

function window_set_topmost(enable) {
    var _buf = window_command_hook_prepare_buffer(9);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_bool, enable);
    return window_set_topmost_raw(buffer_get_address(_buf), 9);
}

function window_get_taskbar_button_visible() {
    var _buf = window_command_hook_prepare_buffer(8);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    return window_get_taskbar_button_visible_raw(buffer_get_address(_buf), 8);
}

function window_set_taskbar_button_visible(show_button) {
    var _buf = window_command_hook_prepare_buffer(9);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_bool, show_button);
    return window_set_taskbar_button_visible_raw(buffer_get_address(_buf), 9);
}

function window_get_clickthrough() {
    var _buf = window_command_hook_prepare_buffer(8);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    return window_get_clickthrough_raw(buffer_get_address(_buf), 8);
}

function window_set_clickthrough(enable_clickthrough) {
    var _buf = window_command_hook_prepare_buffer(9);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_bool, enable_clickthrough);
    return window_set_clickthrough_raw(buffer_get_address(_buf), 9);
}

function window_get_noactivate() {
    var _buf = window_command_hook_prepare_buffer(8);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    return window_get_noactivate_raw(buffer_get_address(_buf), 8);
}

function window_set_noactivate(disable_activation) {
    var _buf = window_command_hook_prepare_buffer(9);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_bool, disable_activation);
    return window_set_noactivate_raw(buffer_get_address(_buf), 9);
}

function window_set_visible_w(visible) {
    var _buf = window_command_hook_prepare_buffer(9);
    buffer_write(_buf, buffer_u64, int64(window_handle()));
    buffer_write(_buf, buffer_bool, visible);
    return window_set_visible_w_raw(buffer_get_address(_buf), 9);
}
