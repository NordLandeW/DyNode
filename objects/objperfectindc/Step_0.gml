/// @description Update perfect indic

var dT = global.timeManager.get_delta();

switch (animState) {
    case 0: // normal
        scaleMul = 1.0;
        break;
    case 1: // expanding
        scaleMul += expandingSpeed * dT / 1000;
        if (scaleMul >= maximumScale) {
            scaleMul = maximumScale;
            animState = 2; // switch to shrinking
        }
        break;
    case 2: // shrinking
        scaleMul -= shrinkingSpeed * dT / 1000;
        if (scaleMul <= 1.0) {
            scaleMul = 1.0;
            animState = 0; // switch to normal
        }
        break;
}

nowTime += dT / 1000;

// Perfect Indicator Fade Out

    if(nowTime < lastTime) {
        alpha = lerp_a(alpha, 1.0, 0.4);
    }
    else if(nowTime < lastTime + PERFECTINDC_ANIMATION_LASTTIME) {
        alpha = animcurve_channel_evaluate(animCurvChan, (nowTime-lastTime) / PERFECTINDC_ANIMATION_LASTTIME);
    }
    else {
        alpha = 0;
    }

// Bloom Fade Out

    if(nowTime < lastTimeBloom) {
        bloomAlpha = lerp_a(bloomAlpha, 1.0, 0.4);
    }
    else if(nowTime < lastTimeBloom + PERFECTINDC_ANIMATION_LASTTIME) {
        bloomAlpha = animcurve_channel_evaluate(animCurvChan, (nowTime-lastTimeBloom) / PERFECTINDC_ANIMATION_LASTTIME);
    }
    else {
        bloomAlpha = 0;
    }

if(objMain.hideScoreboard && !(editor_get_editmode() == 5))
    animTargetAlphaMul = 0;
else
    animTargetAlphaMul = 1;

alphaMul = lerp_a(alphaMul, animTargetAlphaMul, PERFECTINDC_ANIMATION_SPEED);
nowTime = min(nowTime, 99999999);

if(DEBUG_MODE && PERFECTINDC_DEBUG) {
    alpha = 1;
    alphaMul = 1;
}