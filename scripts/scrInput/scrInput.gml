
function InputManager() constructor {
    #macro INPUT_GROUP_DEFAULT_NAME "default"
    #macro INPUT_MOUSE_HOLD_THRESHOLD 300 // Time Threshold to judge if the mouse inputs a hold
    #macro INPUT_MOUSE_DOUBLE_CLICK_THRESHOLD 500
    #macro INPUT_MOUSE_HOLD_DISTANCE_THRESHOLD 15 // Pixels to judge if the mouse inputs a hold
    #macro INPUT_IO_RESET_TIME_THRESHOLD 75
    #macro INPUT_LOCK_RESET_TIME_THRESHOLD 250
    
    frozen = false;

    // In-functions

    static _io_fix = function() {
        // Fix function keys not recognized when io is reset
        var keyToFix = [vk_lcontrol, vk_rcontrol, vk_lalt, vk_ralt, vk_lshift, vk_rshift];
        for(var i=0; i<array_length(keyToFix); i++)
            if(keyboard_check_direct(keyToFix[i]))
                keyboard_key_press(keyToFix[i])
    }
    
    static _ioclear = function () {
        io_clear();
        _io_fix();
        _direct_state_unlock();
        last_mouse_x = 0;
        last_mouse_y = 0;
        
        mouseButtons = [mb_left, mb_right];
        mouseButtonCount = array_length(mouseButtons);
        lastMousePressedPos = [];
        for(var i=0; i<mouseButtonCount; i++)
            lastMousePressedPos[i] = [[0, 0], [0, 0]];
        mouseHoldTime = array_create(mouseButtonCount, 0);
        mouseClick = array_create(mouseButtonCount, -1);
        
        mouseHoldDistanceThreshold = INPUT_MOUSE_HOLD_DISTANCE_THRESHOLD;
        mouseHoldClear = true;
    }
    
    static _pressclear = function () {
        array_fill(mouseClick, -1, 0, 2);
        array_fill(mouseClickDouble, -1, 0, 2);
        array_fill(mouseClickLastTime, 0, 0, 2);
    }
    
    static _holdclear = function () {
        _pressclear();
        mouseHoldClear = true;
        array_fill(mouseHoldTime, 0, 0, 2);
    }

    static _direct_state_lock = function() {
        directStateLock = true;
        directStateLockTimer = 0;
    }
    static _direct_state_unlock = function() {
        directStateLock = false;
        directStateLockTimer = 0;
    }

    static freeze = function() {
        frozen = true;
    }
    static unfreeze = function() {
        frozen = false;
    }
    static is_frozen = function() {
        return frozen;
    }
    
    last_mouse_x = 0;
    last_mouse_y = 0;
    
    mouseButtons = [mb_left, mb_right];
    mouseButtonCount = array_length(mouseButtons);
    lastMousePressedPos = [];
    for(var i=0; i<mouseButtonCount; i++)
        lastMousePressedPos[i] = [[0, 0], [0, 0]];
    mouseHoldTime = array_create(mouseButtonCount, 0);
    mouseClick = array_create(mouseButtonCount, -1);
    mouseClickDouble = array_create(mouseButtonCount, -1);
        // -1 = no input; 0 = not sure if is click (pressing); 1 = is click
    mouseClickLastTime = array_create(mouseButtonCount, 0);
    
    mouseHoldDistanceThreshold = 20;
            // Distance Threshold to judge if a mouse press is a drag (also hold)
    mouseHoldClear = true;
            // Whethere to clear the state of mouse hold
    
    
    // For Input Reset
    windowNFocusTime = 0;
    lowFrameRateFix = false;
    
    // For Input Group
    inputGroup = "default";     // used for changing a code block's input group
    checkGroup = "default";     // the focusing group

    // For Direct State Lock
    directStateLock = false;
    directStateLockTimer = 0;

    _ioclear();
    
    static step = function () {
        if(!keyboard_check(vk_anykey) || directStateLockTimer > INPUT_LOCK_RESET_TIME_THRESHOLD)
            input_direct_state_unlock();
        if(directStateLock)
            directStateLockTimer += delta_time / 1000;

        windowNFocusTime = delta_time / 1000;

        if(windowNFocusTime > INPUT_IO_RESET_TIME_THRESHOLD) {
            if(!lowFrameRateFix) {
                io_clear_diag();
                
                _ioclear();
                show_debug_message_safe("IO Cleared.");
                lowFrameRateFix = true;
            }
        }
        else lowFrameRateFix = false;
        
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
        
        for(var i=0; i<mouseButtonCount; i++) {
            var _nbut = mouseButtons[i];
            if(mouseClick[i] == 1) {
                mouseClick[i] = -1;
            }
            if(mouseClickDouble[i] == 1) {
                mouseClickDouble[i] = -1;
            }
            if(mouse_check_button_pressed(_nbut)) {
                lastMousePressedPos[i][1][0] = lastMousePressedPos[i][0][0];
                lastMousePressedPos[i][1][1] = lastMousePressedPos[i][0][1];
                lastMousePressedPos[i][0][0] = mouse_x;
                lastMousePressedPos[i][0][1] = mouse_y;
                mouseClick[i] = 0;
            }
            
            if(mouse_check_button_released(_nbut) && !mouseHoldClear && mouseClick[i] == 0) {
                var clicking = !(mouseHoldTime[i] > INPUT_MOUSE_HOLD_THRESHOLD)
                if(clicking) {
                    var double_clicking = get_timer() - mouseClickLastTime[i] < 
                            INPUT_MOUSE_DOUBLE_CLICK_THRESHOLD * 1000;
                    if(!double_clicking) {
                        mouseClickLastTime[i] = get_timer();
                        // show_debug_message($"Set timer {mouseClickLastTime[i]}");
                        mouseClick[i] = 1;
                    }
                    else {
                        mouseClick[i] = 1;
                        mouseClickDouble[i] = 1;
                        mouseClickLastTime[i] = 0;
                    }
                }
                else {
                    mouseClick[i] = -1;
                }
            }
            else if(mouseHoldClear) mouseClick[i] = -1;
            
            if(mouse_check_button(_nbut) && !mouseHoldClear) {
                mouseHoldTime[i] += delta_time / 1000;
                if(point_distance(mouse_x, mouse_y, lastMousePressedPos[i][0][0], lastMousePressedPos[i][0][1]) 
                    > mouseHoldDistanceThreshold)
                    mouseHoldTime[i] = 1000000;
            }
            else {
                mouseHoldTime[i] = 0;
            }
        }
        
        if(!mouse_check_button(mb_left) && !mouse_check_button(mb_right))
            mouseHoldClear = false;
    }
}


// Get mouse's delta x from last frame
function mouse_get_delta_x() {
    return mouse_x - global.__InputManager.last_mouse_x;
}
function mouse_get_delta_y() {
    return mouse_y - global.__InputManager.last_mouse_y;
}
function mouse_set_last_pos_l() {
    global.__InputManager.lastMousePressedPos[0][0] = [mouse_x, mouse_y];
}

// Get mouse's delta x from last pressed mb_left frame
function mouse_get_delta_last_x_l() {
    return mouse_x - global.__InputManager.lastMousePressedPos[0][0][0];
}
function mouse_get_delta_last_y_l() {
    return mouse_y - global.__InputManager.lastMousePressedPos[0][0][1];
}

function mouse_get_delta_last_x_r() {
    return mouse_x - global.__InputManager.lastMousePressedPos[1][0][0];
}
function mouse_get_delta_last_y_r() {
    return mouse_y - global.__InputManager.lastMousePressedPos[1][0][1];
}

function mouse_get_last_pos(button) {
    return global.__InputManager.lastMousePressedPos[button][0];
}

function mouse_inbound(x1, y1, x2, y2) {
    if(global.__InputManager.is_frozen()) return false;
    return pos_inbound(mouse_x, mouse_y, x1, y1, x2, y2);
}
function mouse_square_inbound(x, y, a) {
    if(global.__InputManager.is_frozen()) return false;
    return pos_inbound(mouse_x, mouse_y, x-a/2, y-a/2, x+a/2, y+a/2);
}
function mouse_inbound_last_l(x1, y1, x2, y2) {
    if(global.__InputManager.is_frozen()) return false;
    var _nx = global.__InputManager.lastMousePressedPos[0][0][0];
    var _ny = global.__InputManager.lastMousePressedPos[0][0][1];
    return pos_inbound(_nx, _ny, x1, y1, x2, y2);
}
function mouse_square_inbound_last_l(x, y, a) {
    if(global.__InputManager.is_frozen()) return false;
    var _nx = global.__InputManager.lastMousePressedPos[0][0][0];
    var _ny = global.__InputManager.lastMousePressedPos[0][0][1];
    return pos_inbound(_nx, _ny, x-a/2, y-a/2, x+a/2, y+a/2);
}

function mouse_inbound_last_double_l(x1, y1, x2, y2) {
    if(global.__InputManager.is_frozen()) return false;
    var _result = true;
    for(var i=0; i<2; i++) {
        var _nx = global.__InputManager.lastMousePressedPos[0][i][0];
        var _ny = global.__InputManager.lastMousePressedPos[0][i][1];
        _result = _result && pos_inbound(_nx, _ny, x1, y1, x2, y2);
    }
    return _result;
}

function mouse_clear_hold() {
    global.__InputManager._holdclear();
}

function mouse_ishold_l() {
    if(global.__InputManager.is_frozen()) return false;
    return global.__InputManager.mouseHoldTime[0] > INPUT_MOUSE_HOLD_THRESHOLD;
}
function mouse_isclick_l() {
    if(global.__InputManager.is_frozen()) return false;
    return global.__InputManager.mouseClick[0] > 0;
}
function mouse_ishold_r() {
    if(global.__InputManager.is_frozen()) return false;
    return global.__InputManager.mouseHoldTime[1] > INPUT_MOUSE_HOLD_THRESHOLD;
}
function mouse_isclick_r() {
    if(global.__InputManager.is_frozen()) return false;
    return global.__InputManager.mouseClick[1] > 0;
}
function mouse_isclick_double(button) {
    if(global.__InputManager.is_frozen()) return false;
    return global.__InputManager.mouseClickDouble[button] > 0;
}
function mouse_clear_click() {
    global.__InputManager._pressclear();
}

function input_direct_state_lock() {
    global.__InputManager._direct_state_lock();
}

function input_direct_state_unlock() {
    global.__InputManager._direct_state_unlock();
}

function input_direct_state_lock_get() {
    return global.__InputManager.directStateLock;
}

function ctrl_ishold() {
    if(global.__InputManager.is_frozen()) return false;
    return keyboard_check(vk_control);
}
function alt_ishold() {
    if(global.__InputManager.is_frozen()) return false;
    return keyboard_check(vk_alt);
}
function ralt_ishold() {
    if(global.__InputManager.is_frozen()) return false;
    return keyboard_check(vk_ralt);
}
function shift_ishold() {
    if(global.__InputManager.is_frozen()) return false;
    return keyboard_check(vk_shift);
}
function nofunkey_ishold() {
    return !(ctrl_ishold()) && !(alt_ishold()) && !(shift_ishold());
}
function keycheck_ctrl(key) {
    if(global.__InputManager.is_frozen()) return false;
    if(input_direct_state_lock_get()) return false;
    return ctrl_ishold() && keyboard_check(key);
}
function keycheck_down_ctrl(key) {
    if(global.__InputManager.is_frozen()) return false;
    var _result = ctrl_ishold() && keyboard_check_pressed(key);
    if(_result) input_direct_state_lock();
    return _result;
}
function keycheck_shift(key) {
    if(global.__InputManager.is_frozen()) return false;
    if(input_direct_state_lock_get()) return false;
    return shift_ishold() && keyboard_check(key);
}
function keycheck_down_shift(key) {
    if(global.__InputManager.is_frozen()) return false;
    var _result = shift_ishold() && keyboard_check_pressed(key);
    if(_result) input_direct_state_lock();
    return _result;
}

function keycheck(key, nofun = true) {
    if(global.__InputManager.is_frozen()) return false;
    if(input_direct_state_lock_get()) return false;
    return (nofunkey_ishold() || !nofun) && keyboard_check(key);
}
function keycheck_down(key, nofun = true) {
    if(global.__InputManager.is_frozen()) return false;
    return (nofunkey_ishold() || !nofun) && keyboard_check_pressed(key);
}
function keycheck_up(key) {
    if(global.__InputManager.is_frozen()) return false;
    return keyboard_check_released(key);
}
function wheelcheck_up() {
    if(global.__InputManager.is_frozen()) return false;
    return mouse_wheel_up() && nofunkey_ishold();
}
function wheelcheck_down() {
    if(global.__InputManager.is_frozen()) return false;
    return mouse_wheel_down() && nofunkey_ishold();
}
function wheelcheck_up_ctrl() {
    if(global.__InputManager.is_frozen()) return false;
    return mouse_wheel_up() && ctrl_ishold();
}
function wheelcheck_down_ctrl() {
    if(global.__InputManager.is_frozen()) return false;
    return mouse_wheel_down() && ctrl_ishold();
}

function input_group_set(group = INPUT_GROUP_DEFAULT_NAME) {
    global.__InputManager.inputGroup = group;
}
function input_group_reset() {
    global.__InputManager.inputGroup = INPUT_GROUP_DEFAULT_NAME;
}
function input_check_group_set(group = INPUT_GROUP_DEFAULT_NAME) {
    global.__InputManager.checkGroup = group;
}
function input_check_group_reset() {
    global.__InputManager.checkGroup = INPUT_GROUP_DEFAULT_NAME;
}
function input_group_validate(input_group = global.__InputManager.inputGroup) {
    return input_group == global.__InputManager.checkGroup;
}