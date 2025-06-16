/// @description Update scoreboard

if(int64(nowScore) <= scoreLimit || (objMain.hideScoreboard && !(editor_get_editmode() == 5)))
    animTargetAlpha = 0;
else
    animTargetAlpha = 1;

nowScore = lerp_a(nowScore, animTargetScore, animSpeed);
alpha = lerp_a(alpha, animTargetAlpha, animSpeed);

if(expandTimer <= expandTimeLimit) {
    scaleMul = 1.1;
    expandTimer += delta_time / 1000;
} else {
    scaleMul = 1;
}

nowString = string_format(abs(nowScore), preZero, 0);
nowString = string_replace_all(nowString, " ", "0");

if(debug_mode && SCOREBOARD_DEBUG) alpha = 1;