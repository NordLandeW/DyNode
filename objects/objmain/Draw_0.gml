/// @description Draw the Playview

var _nw = BASE_RES_W, _nh = BASE_RES_H;
var _playback = editor_get_editmode() == 5;
var _dynamite = global.style == STYLE.DYNAMITE;

// Draw Bottom
	var _nx = resor_to_x(0.017), _yoffset = BASE_RES_H - targetLineBelow;
	if(global.style == STYLE.DYNAMIX) {
		style_draw_chart_title(chartTitle, _nx + 3, 37 + _yoffset, titleAlpha);
		
		// Draw Difficulty
		draw_sprite_ext(global.difficultySprite[chartDifficulty], 0, 
			_nx, 77 + _yoffset, 0.67, 0.67, 0, c_white, titleAlpha);
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

		dynamix_draw_mixer_lights();
	}
	else if(global.style == STYLE.DYNAMITE) {
		dynamite_draw_judgment_hud();
	}

// Draw Shadows

#macro SHADOW_LAYER_ALPHA (0.9)

    if(global.style == STYLE.DYNAMIX && global.shadowCount > 0) {
        var piano = global.themeAt == 2;
        shadowPingSurf = surface_check(shadowPingSurf, view_wport[0], view_hport[0]);
        surface_set_target(shadowPingSurf);
        gpu_push_state();
        draw_clear_alpha(c_black, 0);
            shader_set(shd_prealpha);
            gpu_set_blendmode_ext(bm_one, bm_inv_src_alpha);
            manually_set_view_size(BASE_RES_W, BASE_RES_H);
            with(objShadow)
                draw_self();
            manually_reset_view_size();
            shader_reset();
        surface_reset_target();
        gpu_pop_state();

        shadowPongSurf = surface_check(shadowPongSurf, view_wport[0], view_hport[0]);
        surface_set_target(shadowPongSurf);
        gpu_push_state();
        draw_clear_alpha(c_black, 0);
            shader_set(shd_prealpha_mul);
            gpu_set_blendmode_ext(bm_one, bm_inv_src_alpha);
            draw_surface_ext(shadowPingSurf, 0, 0, 1, 1, 0, c_white, piano? 1: SHADOW_LAYER_ALPHA);
            shader_reset();
        surface_reset_target();
        gpu_pop_state();

        shadow_layer_draw(shadowPongSurf);
    }

    var _trayMask = false;
    if(_dynamite) {
        _trayMask = dynamite_begin_tray_mask();
        dynamite_draw_footer();
    }

// Draw Notes

	var _piano = global.themeAt == 2 && editor_get_editmode() == 5;
    if(_piano) {
		gpu_set_blendmode(bm_add);
        shader_set(shd_mono);
	}

	if(_playback) {
		global.noteRenderer.render(_dynamite ? dynamiteNoteBound : undefined, _dynamite && dynamiteFxReady);
	}
	else {
		// Draw Holds
		for(var i=0, _cl = array_length(chartNotesArrayActivated[2]); i<_cl; i++)
			chartNotesArrayActivated[2][i].draw_event(false);	// Draw Hold
		if(_dynamite && dynamiteFxReady) global.noteRenderer.draw_fx(DYNAMITE_FX_PASS.FRAGMENTS);
		for(var i=0, _cl = array_length(chartNotesArrayActivated[2]); i<_cl; i++)
			chartNotesArrayActivated[2][i].draw_event(true);	// Draw Edge
		// Dynamite sorts Slide at 3 and Tap at 5.
		var _tap_front = global.style == STYLE.DYNAMITE;
		var _back = chartNotesArrayActivated[_tap_front ? NOTE_TYPE.CHAIN : NOTE_TYPE.NORMAL];
		var _front = chartNotesArrayActivated[_tap_front ? NOTE_TYPE.NORMAL : NOTE_TYPE.CHAIN];
		for(var i=0, _cl = array_length(_back); i<_cl; i++)
			_back[i].draw_event();
		for(var i=0, _cl = array_length(_front); i<_cl; i++)
			_front[i].draw_event();
	}

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

    if(_dynamite && dynamiteFxReady) {
        global.noteRenderer.draw_fx(DYNAMITE_FX_PASS.HIT_FRAME);
        global.noteRenderer.draw_fx(DYNAMITE_FX_PASS.HOLDING_FRAME);
    }
    if(_trayMask) gpu_pop_state();
	
