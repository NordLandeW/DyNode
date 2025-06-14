
var _nw = global.resolutionW, _nh = global.resolutionH;

// Draw Top Progress Bar

    draw_set_color_alpha(c_white, topBarIndicatorA);
    draw_rectangle(0, 0, _nw, topBarMouseH, false);
    if(musicProgress > 0) {
        var _topBarW = round(_nw * musicProgress);
        draw_sprite_stretched_exxt(
        global.sprLazer, 0,
        0, topBarH, _topBarW+1, -26, 0, themeColor, 1.0);
        draw_set_color(c_white); draw_set_alpha(1.0);
        draw_rectangle(0, 0, _topBarW, topBarH, false);
    }
    draw_set_alpha(1.0);

// Draw Shadows

    with(objShadow)
        draw_self();

// Draw Note Particles

    var partSurf = surface_create(_nw, _nh);
    gpu_push_state();
    // gpu_set_colorwriteenable(1, 1, 1, 0);
    // gpu_set_blendmode_ext(bm_one, bm_zero);
    surface_set_target(partSurf);
    draw_clear_alpha(c_black, 0);
    part_system_drawit(partSysNote);
    surface_reset_target();
    gpu_pop_state();
    
    gpu_push_state();
    gpu_set_blendmode(bm_add);
    draw_surface(partSurf, 0, 0);
    surface_free_f(partSurf);
    gpu_pop_state();

// Draw Mixer & Shadow's Position

    for(var i=0; i<2; i++) {
        if(chartSideType[i] == "MIXER") {
                draw_sprite_ext(sprMixer, 0,
                    i*_nw + (i? -1:1) * targetLineBeside, mixerX[i],
                    1, 1, 0, c_white, lerp(0.25, 1, standardAlpha));
            }
    }