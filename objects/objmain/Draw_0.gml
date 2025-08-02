/// @description Draw the Playview

var _nw = BASE_RES_W, _nh = BASE_RES_H;

// Draw Bottom
	var _nx = resor_to_x(0.017), _yoffset = BASE_RES_H - targetLineBelow;
	// Draw Title
	if(has_cjk(chartTitle)) {
		draw_set_halign(fa_left); draw_set_valign(fa_middle);
		
		draw_set_font(global._notoFont);
		// Shadow
		draw_set_color_alpha(c_black, 0.4);
		draw_text(_nx, 44 + _yoffset, chartTitle);
		draw_set_color_alpha(c_white, 1);
		draw_text(_nx, 37 + _yoffset, chartTitle);
		draw_set_alpha(1.0);
	}
	else {
		scribble(chartTitle).starting_format("fOrbitron48s", c_white)
		.align(fa_left, fa_middle)
		.transform(0.7, 0.7)
		.draw(_nx, 42 + _yoffset);
	}
	
	// Draw Difficulty
	draw_sprite_ext(global.difficultySprite[chartDifficulty], 0, 
		_nx, 77 + _yoffset, 0.67, 0.67, 0, c_white, 1);
    	

// Draw targetline

    var _sprlazer = global.sprLazer;
	var _lazerHeight = 25, _lazerBaseAlpha = 0.5;
	var _lazerColor = [themeColor, themeColor];
	var _sideDim = [false, false];

	if(editor_get_editmode() == 5 && nowPlaying) {
		for(var i=0; i<2; i++) {
			if(sideHinterState[i] >= 0 && sideHinterState[i] % 2 == 0) {
				_lazerColor[i] = c_black;
				if(global.themeAt == 2) {
					_lazerColor[i] = c_white;
				}
				_sideDim[i] = true;
			}
		}
	}

	// Side Hinter Dim
	var _dimColor = c_black;
	if(global.themeAt == 2) {
		_dimColor = c_white;
		gpu_set_blendmode(bm_add);
	}
	if(_sideDim[0]) {
		draw_set_color_alpha(_dimColor, 0.6);
		draw_rectangle(0, 0, targetLineBeside, _nh - targetLineBelow, false);
	}
	if(_sideDim[1]) {
		draw_set_color_alpha(_dimColor, 0.6);
		draw_rectangle(_nw - targetLineBeside, 0, _nw, _nh - targetLineBelow, false);
	}
	draw_set_alpha(1.0);
	gpu_set_blendmode(bm_normal);

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
        90, _lazerColor[0], lazerAlpha[1] * _lazerBaseAlpha);
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        targetLineBeside + 1, _nh - targetLineBelow,
        _nh - targetLineBelow, -(targetLineBesideW/2 + _lazerHeight),
        90, _lazerColor[0], lazerAlpha[1] * _lazerBaseAlpha);
    // Light Right
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        _nw - targetLineBeside, _nh - targetLineBelow,
        _nh - targetLineBelow, targetLineBesideW/2 + _lazerHeight,
        90, _lazerColor[1], lazerAlpha[2] * _lazerBaseAlpha);
    draw_sprite_stretched_exxt(
        _sprlazer, 0,
        _nw - targetLineBeside + 1, _nh - targetLineBelow,
        _nh - targetLineBelow, -(targetLineBesideW/2 + _lazerHeight),
        90, _lazerColor[1], lazerAlpha[2] * _lazerBaseAlpha);
    
    
    // Line Left
	if(_sideDim[0])
		draw_set_color_alpha(c_black, 1.0);
	else
    	draw_set_color_alpha(merge_color(themeColor, c_white, lineMix[1]), 1.0);
    draw_rectangle(targetLineBeside - targetLineBesideW/2, 0, 
                    targetLineBeside + targetLineBesideW/2,
                    _nh - targetLineBelow, false);
    // Line Right
	if(_sideDim[1])
		draw_set_color_alpha(c_black, 1.0);
	else
    	draw_set_color_alpha(merge_color(themeColor, c_white, lineMix[2]), 1.0);
    draw_rectangle(_nw - targetLineBeside - targetLineBesideW/2, 0, 
                    _nw - targetLineBeside + targetLineBesideW/2,
                    _nh - targetLineBelow, false);
	// Line Below
	draw_set_color_alpha(merge_color(themeColor, c_white, lineMix[0]), 1.0);
    draw_rectangle(0, _nh - targetLineBelow - targetLineBelowH/2, 
                    _nw, _nh - targetLineBelow + targetLineBelowH/2, false);
    
// Draw Notes

	var _piano = global.themeAt == 2 && editor_get_editmode() == 5;
    if(_piano) {
		gpu_set_blendmode(bm_add);
        shader_set(shd_mono);
	}

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

    if(_piano) {
		gpu_set_blendmode(bm_normal);
        shader_reset();
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
	