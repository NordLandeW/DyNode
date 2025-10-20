
function GUIElement() constructor {
    #macro GUI_MSDF_SCALE 0.8
    
    value = undefined;      // contained value
    name = undefined;       // id
    content = undefined;   // things to draw
    manager = global.__GUIManager;
    
    ds_list_add(manager.elements, self);
    
    // Layout
    width = 100;
    height = 64;
    padding = 16;
    rounding = 10;
    color = 0x111111;
    alpha = 0.85;
    fontSize = 16;
    fontColor = c_white;
    // fontCJK = "sprMsdfNotoSans";
    font = "fMono16N";
    titlePadding = 5;
    
    // States
    inbound = false;
    focus = false;
    active = true;
    pressing = 0;       // probably for animation...
    updateLatency = 0;
    updateColddown = 0;
    
    // Animation Prop
    aspd = 0.4;                 // Animation Speed
    amagnet = 0.01;            // Mouse Magnet Effect
    ashrink = [0.03, -0.07];     // Element Shrink Effect
                                // [0] = inbound; [1] = pressing
    
    x = 0;
    y = 0;
    
    center = {
        x: 0,
        y: 0
    };
    
    // Animation States
    acenter = {
        x: 0,
        y: 0
    };  // Animation Center
    atcenter = {
        x: 0,
        y: 0
    };  // Animation Target Center
    
    ascale = 1;
    atscale = 1;
    
    // Focus part
    static focus_action = function () { };
    static has_focus = function() {
        return focus;
    }
    static set_focus = function() {
        focus = true;
        
        manager.element_in_focus = self;
        
        show_debug_message("Element "+name+" focused.");
    }
    static remove_focus = function() {
        focus = false;
        
        manager.element_in_focus = undefined;
        
        show_debug_message("Element "+name+" unfocused.");
    }
    
    // Animation Effects
    static _a_magnet = function() {
        atcenter = lerp_pos(
                center,
                { x: mouse_x, y: mouse_y},
                amagnet
            );
    }
    static _a_shrink = function(_pressing) {
        atscale = 1 + ashrink[_pressing];
    }
    
    // Inbound part
    static update_inbound = function() {
        inbound = mouse_inbound(x, y, x+width, y+height);
    }
    
    // Value Fetcher
    static get = function() { return value; }
    static set = function(_val) { value = _val; }
    get_value = function() { return value; }
    static update = function() {
        value = get_value();
    }
    
    // Active part
    static activate = function() {
        active = true;
    }
    static deactivate = function() {
        active = false;
    }
    get_active = function() {
        return active;
    }
    static update_active = function() {
        active = get_active();
    }
    
    // Position part
    static set_position = function(_x, _y) {
        x = _x;
        y = _y;
        acenter = center;
        atcenter = center;
        update_position();
    }
    static set_wh = function(_w, _h) {
        width = _w;
        height = _h;
        update_position();
    }
    static set_width = function(_w) {
        set_wh(_w, height);
    }
    static set_height = function(_h) {
        set_wh(width, _h);
    }
    
    static update_position = function() {
        center.x = x + width/2;
        center.y = y + height/2;
    }
    
    // Events
    static step = function() {
        update_inbound();
        update_active();
        update_position();
        if(updateLatency <= 0) {
            update();
        }
        else {
            updateLatency -= delta_time/1000;
        }
        atcenter = center;
        atscale = 1;
        
        if(focus && mouse_check_button_released(mb_left))
            remove_focus();
        
        if(inbound || focus) {
            if(!pressing && mouse_check_button_pressed(mb_left)) {
                pressing = 1;
            }
            else if(pressing && mouse_check_button_released(mb_left))
                pressing = 0;
            
            
            if(active)
                _a_shrink(pressing);
            _a_magnet();
            
            if(mouse_isclick_l()) {
                click();
                mouse_clear_click();
            }
            else if(mouse_ishold_l() && mouse_inbound_last_l(x, y, x+width, y+height)) {
                drag();
                mouse_clear_hold();
            }
        }
        else pressing = 0;
        
        acenter = lerp_a_pos(acenter, atcenter, aspd);
        ascale = lerp_a(ascale, atscale, aspd);
    }
    static click = function() {
        set_focus();
        show_debug_message("Element "+name+" is clicked.");
    }
    static drag = function() {
        set_focus();
        show_debug_message("Element "+name+" is dragged.");
    }
    static listen = function() { }
    
    custom_action = function() { }

    // Draw
    static draw = function() {
        var _x = acenter.x;
        var _y = acenter.y;
        CleanRectangleXYWH(_x, _y, width*ascale, height*ascale)
            .Blend(color, alpha)
            .Border(0, color, 0)
            .Rounding(rounding)
            .Draw();
        
        var _content = content;
        var _scl = ascale * (has_cjk(content) ? 0.65 : 1);
        if(has_cjk(content)) _content = cjk_prefix() + content;
        scribble(_content, "GUI_"+content)
            .starting_format(font, fontColor)
            .align(fa_center, fa_middle)
            .scale(_scl, _scl)
            .draw(_x, _y);
    } 
    static destroy = function() {
        ds_list_delete(manager.elements,
            ds_list_find_index(manager.elements, self)
        );
    }
    
    static __init = function(_id, _x, _y, _content, _action = undefined, _active_check = undefined) {
        if(!is_undefined(_active_check))
            get_active = _active_check;
        
        if(!is_undefined(_action))
            custom_action = _action;
        
        content = _content;
        set_position(_x, _y);
        name = _id;
    }
}

// Button: Click to do an action
// _content: Element description string to draw
// _action: custom action, called when value is changed
function Button(_id, _x, _y, _content, _action = undefined, _active_check = undefined) : GUIElement() constructor {
    __init(_id, _x, _y, _content, _action, _active_check);
    
    static click = function() {
        if(!active) return;
        
        custom_action();
        
        show_debug_message("Button "+name+" is clicked.");
    }
}

// StateButton: Click to do state change and do an action
// _action(state_value): return a new value with the current state
// _state_update(): rewriteget method
function StateButton(_id, _x, _y, _content, _value, _action = undefined, _get_method = undefined, _active_check = undefined) : GUIElement() constructor {
    if(_action == undefined)
        _action = function (val) { return !val; }
    
    __init(_id, _x, _y, _content, _action, _active_check);
    
    value = _value;
    
    if(!is_undefined(_get_method))
        get_value = _get_method;
    
    static click = function() {
        if(!active) return;
        
        value = custom_action(value);
        
        show_debug_message("StateButton "+name+" is clicked.");
    }
    
    static draw = function() {
        var _x = acenter.x;
        var _y = acenter.y;
        var _fcol = fontColor;
        
        if(value) {
            var _col = theme_get().color;
            var _dcol = merge_color(_col, c_black, 0.2);
            CleanRectangleXYWH(_x, _y, width*ascale, height*ascale)
                .Blend4(_col, 1, _col, 1, _dcol, 1, _dcol, 1)
                .Border(0, c_white, 0)
                .Rounding(rounding)
                .Draw();
            _fcol = color_invert(_fcol);
        }
        else {
            CleanRectangleXYWH(_x, _y, width*ascale, height*ascale)
                .Blend(color, alpha)
                .Border(0, color, 0)
                .Rounding(rounding)
                .Draw();
        }
            
        
        var _content = content;
        if(has_cjk(content)) _content = cjk_prefix() + content;
        scribble(content, "GUI_"+content)
            .starting_format(font, c_white)
            .blend(_fcol, 1)
            .align(fa_center, fa_middle)
            .scale(ascale, ascale)
            .draw(_x, _y);
    }
}

// Checkbox: Click to do state change and do an action (in rectangle style)
// _action(state_value): return a new value with the current state
// _state_update(): rewriteget method
function Checkbox(_id, _x, _y, _l, _content, _value, _action = undefined, _get_method = undefined, _active_check = undefined) : StateButton(_id, _x, _y, _content, _value, _action, _get_method, _active_check) constructor {
    set_wh(_l, _l);
    
    #macro CHECKBOX_PADDING 10
    
    static draw = function() {
        var _x = acenter.x;
        var _y = acenter.y;
        var _fcol = fontColor;
        
        if(value) {
            var _col = c_white;
            CleanRectangleXYWH(_x, _y, width*ascale, height*ascale)
                .Blend(_col, 1)
                .Border(0, c_white, 0)
                .Rounding(rounding)
                .Draw();
        }
        else {
            CleanRectangleXYWH(_x, _y, width*ascale, height*ascale)
                .Blend(color, alpha)
                .Border(5, c_white, 1)
                .Rounding(rounding)
                .Draw();
        }
            
        
        if(has_cjk(content)) {
            scribble(cjk_prefix() + content, "GUI_"+content)
                .starting_format(font, c_white)
                .scale(GUI_MSDF_SCALE, GUI_MSDF_SCALE)
                .msdf_shadow(c_black, 0.5, 0, 3, 2)
                .align(fa_left, fa_middle)
                .draw(_x + width/2 + CHECKBOX_PADDING, _y);
        }
        else {
            scribble(content, "GUI_"+content)
                .starting_format(font, c_white)
                .align(fa_left, fa_middle)
                .draw(_x + width/2 + CHECKBOX_PADDING, _y);
        }
        
    }
}

// Bar: Drag or Click to set value
// _range: should be an array including 2 elements = [range_l, range_r]
// _action: custom action, called when value is changed
function Bar(_id, _x, _y, _content, _value, _range, _action = undefined, _active_check = undefined) : GUIElement() constructor {
    __init(_id, _x, _y, _content, _action, _active_check);
    
    value = _value;
    range = _range;
    
    // Animation States
    aval = _value;
    atval = aval;
    
    // Animation Props
    // ashrink = [0.01, -0.01];
    ashrink[1] = -0.03;
    // amagnet = 0.01;
    aspdval = 0.4;
    
    title = true;
    
    static get_progress = function() {
        var _prog = clamp((mouse_x - x)/width, 0, 1);
        return _prog;
    }
    
    static click = function() {
        if(!active) return;
        set(get_progress());
        custom_action();
    }
    
    static listen = function() {
        if(!active) return;
        
        if(get_progress() != value) {
            set(get_progress());
            custom_action();
            
            updateLatency = updateColddown;
        }
    }
    
    static __step = step;
    static step = function() {
        __step();
        atval = value;
        aval = lerp_a(aval, atval, aspdval);
    }
    
    get = function() {
        return lerp(range[0], range[1], value);
    }
    
    static draw = function() {
        var _x = acenter.x;
        var _y = acenter.y;
        CleanRectangleXYWH(_x, _y, width*ascale, height*ascale)
            .Blend(color, alpha)
            .Border(0, color, 0)
            .Rounding(rounding)
            .Draw();
        
        CleanRectangleXYWH(_x - width*ascale*(0.5-aval/2), _y, width*ascale*aval, height*ascale)
            .Blend(color_invert(color), 0.95)
            .Border(0, color_invert(color), 0)
            .Rounding(rounding)
            .Draw();
        
        if(title) {
            if(has_cjk(content)) {
                scribble(cjk_prefix() + content)
                    .starting_format(font, c_white)
                    .msdf_shadow(c_black, 0.5, 0, 3, 2)
                    .scale(0.8, 0.8)
                    .align(fa_left, fa_bottom)
                    .draw(x, y - titlePadding);
            }
            else
                scribble(content)
                    .starting_format(font, c_white)
                    .align(fa_left, fa_bottom)
                    .draw(x, y - titlePadding);
        }
    }
    
    static update = function() {
        value = clamp(get_value(), 0, 1);
    }
}

function GUIManager() constructor {
    global.__GUIManager = self;
    
    elements = ds_list_create();
    
    element_in_focus = undefined;
    
    static step = function() {
        if(mouse_isclick_l()) {
            if(!is_undefined(element_in_focus))
                element_in_focus.remove_focus();
        }
        
        for(var i=ds_list_size(elements)-1; i>=0; i--) {
            elements[| i].step();
        }
        
        if(element_in_focus != undefined)
            element_in_focus.listen();
    }
    
    static draw = function() {
        for(var i=ds_list_size(elements)-1; i>=0; i--) {
            elements[| i].draw();
        }
    }
    
    static destroy = function() {
        for(var i=ds_list_size(elements)-1; i>=0; i--) {
            delete elements[| i];
        }
        ds_list_destroy(elements);
        delete global.__GUIManager;
        global.__GUIManager = undefined;
    }
}

function gui_manager_create() {
    return new GUIManager();
}

function gui_manager_destroy() {
    if(global.__GUIManager != undefined)
        global.__GUIManager.destroy();
}