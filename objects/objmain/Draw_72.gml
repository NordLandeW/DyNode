/// @description Draw the Background & Beat Lines

var _nw = BASE_RES_W, _nh = BASE_RES_H;

// Draw Background

	// Image
    if(bgImageSpr != -1) {
        var _spr = bgImageSpr;
        var _sw = sprite_get_width(_spr), _sh = sprite_get_height(_spr);
        var _scl = max(_nw / _sw, _nh / _sh); // Centre & keep ratios
        var _sx = (_nw - _sw * _scl) / 2, _sy = (_nh - _sh * _scl) / 2;

        draw_sprite_ext(_spr, 0, _sx, _sy, _scl, _scl, 0, c_white, 1);
    }
    else {
        draw_clear(c_white);
    }
    
    // Video
    if(bgVideoAlpha > EPS && !global.recordManager.is_recording()) 
		safe_video_draw(0, 0, bgVideoAlpha);
	else if(!bgVideoPaused && editor_get_editmode() != 5)
		safe_video_pause();

	// Draw bottom blured bg
    
    if(bgBlured) {
        var _surf = kawase_get_surface(kawaseArr);
        surface_set_target(_surf);
            draw_surface(application_surface, 0, targetLineBelow - _nh);
        surface_reset_target();
        kawase_blur(kawaseArr);
        draw_surface(_surf, 0, _nh - targetLineBelow);
    }
    
    // Dim background
	    draw_set_color(c_black);
	    draw_set_alpha(bgDim);
	    draw_rectangle(0, 0, _nw, _nh, false);
	    draw_set_alpha(1.0);
        
        draw_set_color(c_black);
        draw_set_alpha(bottomDim);
        draw_rectangle(0, _nh - targetLineBelow, _nw, _nh, false);
        draw_set_alpha(1.0);

	// Draw Bottom Right
    
    var _nxx = resor_to_x((1920 - 80 - 30) / 1920), _nww = 180;
    for(var i = 0; i < 8; i++) {
        draw_sprite_ext(
            i==0?sprBottomSignBlue:sprBottomSignBlack,
            0,
            _nxx - i * _nww / 2,
            (i & 1) == 0? _nh : _nh - targetLineBelow,
            1,
            1,
            (i & 1) == 0? 180 : 0,
            c_white,
            1
            );
    }

// Draw Faint Color

    draw_sprite_stretched_exxt(
        global.sprLazer, 0,
        0, _nh - targetLineBelow, _nw, BASE_RES_H*0.75,
        0, themeColor, bgFaintAlpha * animCurvFaintEval * 0.65);

// Draw Pause Shadow

	// Draw Shadow
	var _sw = sprite_get_width(global.sprPauseShadow);
	var _sh = sprite_get_height(global.sprPauseShadow);
	draw_sprite_ext(global.sprPauseShadow, 0, 
		0, blackBarFromTop - blackBarHeight/2, 
		resor_to_x(1)/_sw, blackBarHeight/_sh, 0, c_black, standardAlpha);
	
	// Draw Pause Icon
	
	if(!showDebugInfo || global.recordManager.is_recording()) {
		draw_sprite_ext(sprPauseBar, 0,
			resor_to_x(0.5) - pauseBarIndent / 2, blackBarFromTop,
			1, 1, 0, c_white, standardAlpha);
		draw_sprite_ext(sprPauseBar, 0,
			resor_to_x(0.5) + pauseBarIndent / 2, blackBarFromTop,
			1, 1, 0, c_white, standardAlpha);
	}