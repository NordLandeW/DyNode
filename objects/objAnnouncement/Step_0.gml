
if(annoState != ANNO_STATE.PROCESSING)
    timer += global.timeManager.get_delta_default() / 1000;
anniTimer += global.timeManager.get_delta_default() / 1000;

if(lastTime < timer) {
    image_alpha -= objManager.animAnnoSpeed;
    if(image_alpha < 0.01)
        instance_destroy();
}
else {
    image_alpha = lerp_a(image_alpha, animTargetAlpha, 0.3);
}

// Background color update
update_color();
bgColor = lerp_timedep_any(bgColor, targetBgColor, 0.1,
    function(a, b, t) { return a.lerp(a, b, t); },
    function(a, b) { return a.eqls(b); }, 60);

// Follow the breathing curve directly; smooth only after breathing stops.
if(annoType == ANNO_TYPE.TASK && annoState == ANNO_STATE.PROCESSING) {
    var _breath = sin(anniTimer * 2 * pi / ANNO_BORDER_ANIMATION_TIME);
    borderThick = _breath * 2 + 2;
    borderAlpha = _breath * 0.5 + 0.5;
}
else {
    borderThick = lerp_timedep(borderThick, 0, 0.1, 60);
    borderAlpha = lerp_timedep(borderAlpha, 0, 0.1, 60);
}

nowShiftY = lerp_a(nowShiftY, targetY, 0.3);
