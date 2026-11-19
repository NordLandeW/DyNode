/// @description Var init

#macro SCOREBOARD_WIDTH 420
#macro SCOREBOARD_NUMBER_HEIGHT 65
#macro SCOREBOARD_NUMBER_WIDTH 53
#macro SCOREBOARD_NUMBER_PADDING 8
#macro SCOREBOARD_NUMBER_SPRITE_PADDING 21

#macro SCOREBOARD_DEBUG false

// In-Variables

scale = 0.5;
scaleMul = 1.0;
preZero = 7;
nowScore = 0;
nowString = "";
align = fa_middle;

scoreLimit = 0;
alpha = 0;

animSpeed = 0.2;
animTargetScore = 0;
animTargetScaleMul = 1.0;
animTargetAlpha = 0;

expandTimer = 999;
expandTimeLimit = 2.5/60 * 1000; // in ms

sprWidth = sprite_get_width(sprNumber)
sprHeight = sprite_get_height(sprNumber);
shorten = 20; // total shorten pixels for each number in lr side

// Recaculate the scale with macro
scale = SCOREBOARD_NUMBER_WIDTH / (sprWidth - SCOREBOARD_NUMBER_SPRITE_PADDING * 2);

visible = true;

function _update_score(_scr, _hit = true, _force = false) {
    if(_hit) 
        expandTimer = 0;
    animTargetScore = _scr;
    if(_force) nowScore = _scr;
}

function reset() {
    alpha = 0;
    scaleMul = 1.0;
}

lastDynamiteValue = -1;
dynamiteText = undefined;
dynamiteLabelX = 0;

/// @description Draw an immediate AP counter on the pause strip in the original BPG face.
function draw_dynamite_counter(_value, _isScore) {
    if(lastDynamiteValue != _value) {
        var _text = string(_value);
        if(_isScore) {
            var _padding = 7 - string_length(_text);
            if(_padding > 0) _text = "[alpha,0.502]" + string_repeat("0", _padding) + "[/alpha]" + _text;
        } else {
            // At 80px the font advances 54px per digit, then 27px for the full-size space.
            dynamiteLabelX = 256.7 + 54 * string_length(_text) + 27;
        }
        dynamiteText = scribble(_text, _isScore ? "dynamite_score" : "dynamite_combo")
            .starting_format("mDynamiteTitle", c_white)
            .align(_isScore ? fa_center : fa_left, fa_top)
            .transform(54 / 30, 80 / 44);
        lastDynamiteValue = _value;
    }
    // Scribble lays mixed sizes out around their centres; Unity uses a shared baseline.
    var _baseline = 152;
    dynamiteText.draw(_isScore ? 1511 : 256.7, _baseline - 80);
    if(!_isScore) {
        scribble("AP Combo", "dynamite_label").starting_format("mDynamiteTitle", c_white)
            .align(fa_left, fa_top).transform(1.08, 48 / 44)
            .draw(dynamiteLabelX, _baseline - 48);
    }
}
