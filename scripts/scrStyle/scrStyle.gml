
enum STYLE {
    DYNAMIX,
    DYNAMITE
}

#macro STYLE_COUNT (2)
#macro DYNAMITE_SIDE_SPEED_SCALE (0.9)
global.STYLE_NAMES = ["Dynamix", "Dynamite"];

function style_init() {
    global.style = STYLE.DYNAMIX;
}

function style_next() {
    if(global.style == STYLE.DYNAMITE) DyCore_prepare_dynamite_fx(0, 0, 0, 0, false, false, false);
    global.style = (global.style + 1) % STYLE_COUNT;

    announcement_set("style", global.STYLE_NAMES[global.style]);
}

/// @description Target travel speed of the side lanes relative to the front lane.
function style_side_speed_scale() {
    return global.style == STYLE.DYNAMITE ? DYNAMITE_SIDE_SPEED_SCALE : 1;
}

/// @description Inline Tap, Chain and Hold artwork at the chart stats' text size.
function style_note_stat_icons() {
    static _dynamix = ["[sprNote]", "[scale,0.4][sprChain][/s]", "[scale,0.4][sprHoldEdge][/s]"];
    static _dynamite = ["[scale,0.17][sprDynamiteTap][/s]", "[scale,0.16][sprDynamiteChainIcon][/s]", "[scale,0.17][sprDynamiteHoldIcon][/s]"];
    return global.style == STYLE.DYNAMITE ? _dynamite : _dynamix;
}

/// Tile the playfield artwork from the lower left, clipping the last row and column to the band.
function style_draw_stripe_band(_sprite, _top, _bottom, _tile_height, _alpha) {
    if(_alpha <= 0 || _top >= _bottom) return;
    var _uv = sprite_get_uvs(_sprite, 0);
    var _tile_width = sprite_get_width(_sprite) * (100 / 75); // Canvas reference PPU / sprite PPU.
    var _trim_top = _uv[5] * _tile_height / sprite_get_height(_sprite);
    var _saved_height = _uv[7] * _tile_height;
    var _tex = sprite_get_texture(_sprite, 0);
    draw_primitive_begin_texture(pr_trianglelist, _tex);
    for(var _row = _bottom - _tile_height; _row + _tile_height > _top; _row -= _tile_height) {
        var _row_top = _row + _trim_top;
        var _y1 = max(_top, _row_top), _y2 = _row_top + _saved_height;
        if(_y1 >= _y2) continue;
        // Immediate-mode texture coordinates are relative to the packed sprite, not the atlas.
        var _v1 = (_y1 - _row_top) / _saved_height;
        for(var _x1 = 0; _x1 < BASE_RES_W; _x1 += _tile_width) {
            var _x2 = min(BASE_RES_W, _x1 + _tile_width);
            var _u2 = (_x2 - _x1) / _tile_width;
            draw_vertex_texture_color(_x1, _y1, 0, _v1, c_white, _alpha);
            draw_vertex_texture_color(_x2, _y1, _u2, _v1, c_white, _alpha);
            draw_vertex_texture_color(_x1, _y2, 0, 1, c_white, _alpha);
            draw_vertex_texture_color(_x2, _y1, _u2, _v1, c_white, _alpha);
            draw_vertex_texture_color(_x2, _y2, _u2, 1, c_white, _alpha);
            draw_vertex_texture_color(_x1, _y2, 0, 1, c_white, _alpha);
        }
    }
    draw_primitive_end();
}

/// @description Resolve a glyph missing from the Dynamite title atlas.
/// @param {String} _char Single Unicode character.
function style_dynamite_title_fallback(_char) {
    if(is_undefined(global._sourceHanTitleFont)) {
        global._sourceHanTitleFont = font_add("fonts/SOURCEHANSANSCN-NORMAL.otf", 33, false, false, 32, 65535);
        if(font_exists(global._sourceHanTitleFont)) font_enable_sdf(global._sourceHanTitleFont, true);
    }
    // A missing glyph has no cache entry. Probe just the requested character, not the whole OTF.
    if(font_exists(global._sourceHanTitleFont) && is_struct(font_cache_glyph(global._sourceHanTitleFont, ord(_char))))
        return global._sourceHanTitleFont;
    return global._notoFont;
}

/// @description Lay out contiguous runs in the theme font or its dynamic fallbacks.
/// @param {String} _text Chart title, as literal text.
/// @param {Bool} _dynamite Use the Dynamite font chain and metrics.
function style_layout_chart_title(_text, _dynamite) {
    var _primary = _dynamite ? "mDynamiteTitle" : "mOrbitron";
    var _runs = [], _last = undefined;
    var _oldFont = draw_get_font();
    for(var i = 1, n = string_length(_text); i <= n; ++i) {
        var _char = string_char_at(_text, i), _font = -1;
        if(!scribble_font_has_character(_primary, _char))
            _font = _dynamite ? style_dynamite_title_fallback(_char) : global._notoFont;
        if(is_undefined(_last) || _last.font != _font) {
            _last = { font: _font, text: "", element: undefined, x: 0, dy: 0, scale: 1 };
            array_push(_runs, _last);
        }
        _last.text += _char;
    }

    var _x = 0;
    for(var i = 0; i < array_length(_runs); ++i) {
        var _r = _runs[i];
        _r.x = _x;
        if(_r.font == -1) {
            _r.element = scribble(_r.text, "chart_title:" + _primary).starting_format(_primary, c_white)
                .ignore_command_tags(true).align(fa_left, _dynamite ? fa_top : fa_middle);
            if(!_dynamite) _r.element.scale(1.5).msdf_shadow(c_black, 0.4, 0, 3);
            _x += _r.element.get_width() - 1; // Scribble includes both edges in its bounding box.
        } else {
            draw_set_font(_r.font);
            if(_dynamite) {
                // Match BPG's 44px design em and baseline. OTF hhea ascents are 0.88em / 1.16em.
                _r.scale = 44 / max(1, string_width("中"));
                _r.dy = 44 * (1 - (_r.font == global._notoFont ? 1.16 : 0.88));
            }
            _x += string_width(_r.text) * _r.scale;
        }
    }
    draw_set_font(_oldFont);
    return _runs;
}

/// @description Draw the playfield title at the style's native anchor.
/// @param {String} _text Chart title.
/// @param {Real} _x Left anchor.
/// @param {Real} _y Top (Dynamite) or center (Dynamix) anchor.
/// @param {Real} _alpha Title opacity.
function style_draw_chart_title(_text, _x, _y, _alpha) {
    static _title = undefined, _style = undefined, _runs = [];
    var _dynamite = global.style == STYLE.DYNAMITE;
    if(_title != _text || _style != global.style) {
        for(var i = 0; i < array_length(_runs); ++i) {
            if(!is_undefined(_runs[i].element)) _runs[i].element.flush();
        }
        _runs = style_layout_chart_title(_text, _dynamite);
        _title = _text;
        _style = global.style;
    }

    var _oldFont = draw_get_font(), _oldColor = draw_get_color(), _oldAlpha = draw_get_alpha();
    var _oldH = draw_get_halign(), _oldV = draw_get_valign();
    draw_set_halign(fa_left);
    draw_set_valign(_dynamite ? fa_top : fa_middle);
    for(var i = 0; i < array_length(_runs); ++i) {
        var _r = _runs[i], _rx = _x + _r.x;
        if(!is_undefined(_r.element)) {
            _r.element.blend(c_white, _alpha).draw(_rx, _y);
        } else {
            draw_set_font(_r.font);
            if(!_dynamite) {
                draw_set_color_alpha(c_black, 0.4 * _alpha);
                draw_text(_rx, _y + 7, _r.text);
            }
            draw_set_color_alpha(c_white, _alpha);
            draw_text_transformed(_rx, _y + _r.dy, _r.text, _r.scale, _r.scale, 0);
        }
    }
    draw_set_font(_oldFont);
    draw_set_halign(_oldH);
    draw_set_valign(_oldV);
    draw_set_color_alpha(_oldColor, _oldAlpha);
}
