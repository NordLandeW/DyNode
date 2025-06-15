/// @description Draw the Playview

var _nw = global.resolutionW, _nh = global.resolutionH;

// Draw Bottom
    
    if(!surface_exists(bottomInfoSurf)) {
    	bottomInfoSurf = surface_create(global.resolutionW, targetLineBelow);
    }
    
    surface_set_target(bottomInfoSurf);
    	
    		draw_clear_alpha(c_black, 0);
    		var _nx = resor_to_x(0.017);
	    	// Draw Title
			if(has_cjk(chartTitle)) {
				draw_set_halign(fa_left); draw_set_valign(fa_middle);
				
				draw_set_font(global._notoFont);
				// Shadow
				draw_set_color_alpha(c_black, 0.4);
				draw_text(_nx, 44 * global.scaleYAdjust, chartTitle);
				draw_set_color_alpha(c_white, 1);
				draw_text(_nx, 37 * global.scaleYAdjust, chartTitle);
				draw_set_alpha(1.0);
			}
			else {
				scribble(chartTitle).starting_format("fOrbitron48s", c_white)
		        .align(fa_left, fa_middle)
			    .transform(global.scaleXAdjust * 0.7, global.scaleYAdjust * 0.7)
			    .draw(_nx, 42 * global.scaleYAdjust);
			}
		    
		    // Draw Difficulty
		    draw_sprite_ext(global.difficultySprite[chartDifficulty], 0, 
		        _nx, 77 * global.scaleYAdjust,
		        0.67 * global.scaleXAdjust, 0.67 * global.scaleYAdjust, 0, c_white,
		        1);
	    
    surface_reset_target();
    
    draw_surface_ext(bottomInfoSurf, 0, global.resolutionH - targetLineBelow,
    	1.0, 1.0, 0, c_white, titleAlpha);
    	

// Draw targetline

    var _sprlazer = global.sprLazer;
	var _lazerHeight = 25, _lazerBaseAlpha = 0.5;
    // Light Below
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        0, _nh - targetLineBelow,
        _nw, targetLineBelowH/2 + _lazerHeight,
        0, themeColor, lazerAlpha[0] * _lazerBaseAlpha);
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        0, _nh - targetLineBelow + 1,
        _nw, -(targetLineBelowH/2 + _lazerHeight),
        0, themeColor, lazerAlpha[0] * _lazerBaseAlpha);
    // Light Left
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        targetLineBeside, _nh - targetLineBelow,
        _nh - targetLineBelow, targetLineBesideW/2 + _lazerHeight,
        90, themeColor, lazerAlpha[1] * _lazerBaseAlpha);
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        targetLineBeside + 1, _nh - targetLineBelow,
        _nh - targetLineBelow, -(targetLineBesideW/2 + _lazerHeight),
        90, themeColor, lazerAlpha[1] * _lazerBaseAlpha);
    // Light Right
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        _nw - targetLineBeside, _nh - targetLineBelow,
        _nh - targetLineBelow, targetLineBesideW/2 + _lazerHeight,
        90, themeColor, lazerAlpha[2] * _lazerBaseAlpha);
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        _nw - targetLineBeside + 1, _nh - targetLineBelow,
        _nh - targetLineBelow, -(targetLineBesideW/2 + _lazerHeight),
        90, themeColor, lazerAlpha[2] * _lazerBaseAlpha);
    
    
    // Line Left
    draw_set_color_alpha(merge_color(themeColor, c_white, lineMix[1]), 1.0);
    draw_rectangle(targetLineBeside - targetLineBesideW/2, 0, 
                    targetLineBeside + targetLineBesideW/2,
                    _nh - targetLineBelow, false);
    // Line Right
    draw_set_color_alpha(merge_color(themeColor, c_white, lineMix[2]), 1.0);
    draw_rectangle(_nw - targetLineBeside - targetLineBesideW/2, 0, 
                    _nw - targetLineBeside + targetLineBesideW/2,
                    _nh - targetLineBelow, false);
	// Line Below
	draw_set_color_alpha(merge_color(themeColor, c_white, lineMix[0]), 1.0);
    draw_rectangle(0, _nh - targetLineBelow - targetLineBelowH/2, 
                    _nw, _nh - targetLineBelow + targetLineBelowH/2, false);
    
// Draw Notes
	
	if(instance_count <= INSTANCE_OPTI_THRESHOLD) {
		// Draw Holds
		for(var i=0, _cl = array_length(chartNotesArrayActivated[2]); i<_cl; i++)
			chartNotesArrayActivated[2][i].draw_event(false);	// Draw Hold
		for(var i=0, _cl = array_length(chartNotesArrayActivated[2]); i<_cl; i++)
			chartNotesArrayActivated[2][i].draw_event(true);	// Draw Edge
		// Draw Notes
		for(var i=0, _cl = array_length(chartNotesArrayActivated[0]); i<_cl; i++)
			chartNotesArrayActivated[0][i].draw_event();
		// Draw Chains
		for(var i=0, _cl = array_length(chartNotesArrayActivated[1]); i<_cl; i++)
			chartNotesArrayActivated[1][i].draw_event();
	}

	// Draw Attaching Notes
	if(is_array(objEditor.editorNoteAttaching))
		for(var i=0; i<array_length(objEditor.editorNoteAttaching); i++)
			with(objEditor.editorNoteAttaching[i])
				if(attaching) {
					if(noteType < 2)
						draw_event();
					else if(noteType == 2) {
						draw_event(false);
						draw_event(true);
					}
				}
	