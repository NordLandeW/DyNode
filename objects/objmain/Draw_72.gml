/// @description Draw the Background & Beat Lines

var _nw = global.resolutionW, _nh = global.resolutionH;

// Draw Background

	// Image
    if(bgImageSpr != -1) {
        draw_sprite(bgImageSpr, 0, _nw/2, _nh/2);
    }
    else {
        draw_clear(c_white);
    }
    
    // Video
    if(bgVideoAlpha > EPS) 
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
        0, _nh - targetLineBelow, _nw, global.resolutionH*0.75,
        0, themeColor, bgFaintAlpha * animCurvFaintEval * 0.65);

// Draw Pause Shadow

	// Draw Shadow
	var _sw = sprite_get_width(global.sprPauseShadow);
	var _sh = sprite_get_height(global.sprPauseShadow);
	draw_sprite_ext(global.sprPauseShadow, 0, 
		0, blackBarFromTop - blackBarHeight/2, 
		resor_to_x(1)/_sw, blackBarHeight/_sh, 0, c_black, standardAlpha);
	
	// Draw Pause Icon
	
	if(!showDebugInfo) {
		draw_sprite_ext(sprPauseBar, 0,
			resor_to_x(0.5) - pauseBarIndent / 2, blackBarFromTop,
			1, 1, 0, c_white, standardAlpha);
		draw_sprite_ext(sprPauseBar, 0,
			resor_to_x(0.5) + pauseBarIndent / 2, blackBarFromTop,
			1, 1, 0, c_white, standardAlpha);
	}