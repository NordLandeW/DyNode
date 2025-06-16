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