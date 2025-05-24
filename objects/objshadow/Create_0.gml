
// In-Variables

visible = false;
animState = 1.0;
animTarget = 0.0;
animTargetX = 0.8;
animTargetY = 0.9;
animCurvChan = animcurve_get_channel(curvShadowFade, "curve1");
animTime = 400;
animLast = 0;

originalWidth = sprite_get_width(sprite_index);
nowWidth = 1;
extraWidth = 250;

nowScaleX = 1;

alphaMul = 1.0;

function _prop_init() {
    nowScaleX = (nowWidth + extraWidth) / originalWidth;
    image_xscale = nowScaleX;
}

global.shadowCount ++;