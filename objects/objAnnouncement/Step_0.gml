
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

nowShiftY = lerp_a(nowShiftY, targetY, 0.3);
