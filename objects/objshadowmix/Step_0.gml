
nowScaleX = 1;

animLast += global.timeManager.get_delta() / 1000;

animState = animcurve_channel_evaluate(animCurvChan, min(animLast/animTime, 1));
image_xscale = lerp(nowScaleX, nowScaleX * animTargetX, 1-animState);
image_yscale = lerp(1, animTargetY, 1-animState);
image_alpha = lerp(1, 0, 1-animState) * alphaMul;

animLast = min(animLast, 9999);