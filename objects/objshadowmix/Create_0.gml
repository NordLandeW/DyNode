/// @description Track mixer hits for the judgment HUD.
animCurvChan = animcurve_get_channel(curvShadowFade, "curve1");
animTargetX = 0.8;
animTargetY = 0.9;

animTime = 250;
animLast = 9999;
image_alpha = 0;

alphaMul = 1;

_hit = function () {
    animLast = 0;
}

depth = -80000000;