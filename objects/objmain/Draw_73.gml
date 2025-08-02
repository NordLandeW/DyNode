
var _nw = BASE_RES_W, _nh = BASE_RES_H;

// Draw Top Progress Bar

    draw_set_color_alpha(c_white, topBarIndicatorA);
    draw_rectangle(0, 0, _nw, topBarMouseH, false);
    if(musicProgress > 0) {
        var _topBarW = round(_nw * musicProgress);
        draw_sprite_stretched_exxt(
        global.sprLazer, 0,
        0, topBarH, _topBarW+1, -26, 0, themeColor, 0.5);
        draw_set_color(c_white); draw_set_alpha(1.0);
        draw_rectangle(0, 0, _topBarW, topBarH, false);
    }
    draw_set_alpha(1.0);

// Draw Shadows

    if(global.themeAt == 2) {
        shader_set(shd_mono);
        gpu_set_blendmode(bm_add);
    }
    else if(global.themeAt > 0) {
        // Else if a customized theme
        shader_set(shd_hsv_trans);
        var hsv = theme_get_color_hsv();
        shader_set_uniform_q("u_hue", hsv[0]);
        shader_set_uniform_q("u_saturation", hsv[1]);
    }
    with(objShadow)
        draw_self();
    if(global.themeAt > 0) {
        shader_reset();
        gpu_set_blendmode(bm_normal);
    }

// Draw Note Particles

    var partSurf = surface_create(_nw, _nh);
    gpu_push_state();
    // gpu_set_colorwriteenable(1, 1, 1, 0);
    surface_set_target(partSurf);
    draw_clear_alpha(c_black, 1);
        // gpu_set_blendmode_ext(bm_one, bm_inv_src_alpha);
        // gpu_set_blendmode(bm_max);      // maybe not valid but looks ok :(
        gpu_set_blendmode(bm_add);
        shader_set(shd_prealpha);
        part_system_drawit(partSysNote);
        shader_reset();
    surface_reset_target();
    gpu_pop_state();
    
    gpu_push_state();
    gpu_set_blendmode(bm_add);
    shader_set(shd_unprealpha);
    draw_surface(partSurf, 0, 0);
    shader_reset();
    surface_free_f(partSurf);
    gpu_pop_state();

// Draw Mixer & Shadow's Position

    // If is piano theme
    if(global.themeAt == 2)
        shader_set(shd_mono);
    else if(global.themeAt > 0) {
        // Else if a customized theme
        shader_set(shd_hsv_trans);
        var hsv = theme_get_color_hsv();
        shader_set_uniform_q("u_hue", hsv[0]);
        shader_set_uniform_q("u_saturation", hsv[1]);
    }
    for(var i=0; i<2; i++) {
        if(chartSideType[i] == "MIXER") {
                draw_sprite_ext(sprMixer, 0,
                    i*_nw + (i? -1:1) * targetLineBeside, mixerX[i],
                    1, 1, 0, c_white, lerp(0.25, 1, standardAlpha));
            }
    }
    if(global.themeAt > 0)
        shader_reset();