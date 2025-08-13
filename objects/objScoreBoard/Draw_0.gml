/// @description Draw scoreboard

var _len = string_length(nowString);
var _x = x + (align == fa_right ? 0 : 1) * SCOREBOARD_WIDTH / 2;
var _y = y + SCOREBOARD_NUMBER_HEIGHT / 2;
var _w = (SCOREBOARD_NUMBER_WIDTH + SCOREBOARD_NUMBER_PADDING) * scaleMul;

if(align == fa_middle)
    _x -= _w * (_len - 1) / 2;
else if(align == fa_right)
    _x -= _w * (_len - 1) + SCOREBOARD_NUMBER_WIDTH / 2;

for(var _i = 1; _i <= _len; _i ++) {
    draw_sprite_ext(sprNumber, int64(string_char_at(nowString, _i)), _x, _y, 
         scale * scaleMul,  
         scale * scaleMul, 0, c_white, alpha);
    if(DEBUG_MODE && SCOREBOARD_DEBUG) {
        draw_set_color(c_yellow);
        draw_set_alpha(1);
        draw_circle(_x, _y, 5, false);
    }
    _x += _w;
}

if(DEBUG_MODE && SCOREBOARD_DEBUG) {
    draw_set_color(c_red);
    draw_set_alpha(1);
    draw_circle(x, y, 5, false);
    var _x = x + (align == fa_right ? 0 : 1) * SCOREBOARD_WIDTH / 2;
    var _y = y + SCOREBOARD_NUMBER_HEIGHT / 2;
    draw_circle(_x, _y, 5, false);
}