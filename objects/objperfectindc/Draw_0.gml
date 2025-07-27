/// @description Draw the indicator

var _scl = scale * scaleMul;

var _nx = x + PERFECTINDC_SPRITE_ALPHABETS_WIDTH / 2;
var _ny = y + PERFECTINDC_SPRITE_ALPHABETS_HEIGHT / 2;

// If is piano theme
var _col = c_white;
if(global.themeAt == 2) {
    gpu_set_blendmode(bm_add);
    shader_set(shd_mono);
    _col = c_gray;
}
else if(global.themeAt > 0) {
    // Else if a customized theme
    shader_set(shd_hsv_trans);
    var hsv = theme_get_color_hsv();
    shader_set_uniform_q("u_hue", hsv[0]);
    shader_set_uniform_q("u_saturation", hsv[1]);
}


draw_sprite_ext(sprPerfect, 0, _nx, _ny, _scl * global.scaleXAdjust, _scl * global.scaleYAdjust, 0, _col, alpha * alphaMul);
draw_sprite_ext(sprPerfectBloom, 0, _nx, _ny, _scl * global.scaleXAdjust, _scl * global.scaleYAdjust, 0, _col, 1 * bloomAlpha * alpha * alphaMul);

if(global.themeAt > 0) {
    gpu_set_blendmode(bm_normal);
    shader_reset();
}

if(DEBUG_MODE && PERFECTINDC_DEBUG) {
    draw_set_color(c_red);
    draw_set_alpha(1);
    draw_circle(x, y, 5, false);
    draw_circle(_nx, _ny, 5, false);
}