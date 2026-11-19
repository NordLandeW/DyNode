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
    if(bgVideoAlpha > EPS) 
		dyc_video_draw(0, 0, bgVideoAlpha);
	else if(dyc_video_is_playing() && editor_get_editmode() != 5)
		dyc_video_pause();

    // Dim background
	    draw_set_color(c_black);
	    draw_set_alpha(bgDim);
	    draw_rectangle(0, 0, _nw, _nh, false);
	    draw_set_alpha(1.0);
        
        
    if(global.style == STYLE.DYNAMIX) {
        // Dim Bottom
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
    }
    else if(global.style == STYLE.DYNAMITE) {
        var _playback = editor_get_editmode() == 5;
        var _trayMask = dynamite_begin_tray_mask();
        var _bar_center = 124, _bar_height = 84;
        style_draw_stripe_band(sprDynamitePauseStripe, _bar_center - _bar_height / 2,
            _bar_center + _bar_height / 2, sprite_get_height(sprDynamitePauseStripe) * (100 / 75),
            standardAlpha * (70 / 255));
        if(!showDebugInfo || global.recordManager.is_recording()) {
            var _scale = 93.24 / sprite_get_height(sprDynamitePause);
            draw_sprite_ext(sprDynamitePause, 0, _nw / 2, _bar_center - 4,
                _scale, _scale, 0, c_white, standardAlpha);
        }
        if(_playback || !hideScoreboard) {
            with(objScoreBoard) {
                if(visible) draw_dynamite_counter(floor(animTargetScore), id == other.scbLeft);
            }
        }

        global.noteRenderer.begin_dynamite_notes(_playback);
        if(!_playback) {
            var _holds = chartNotesArrayActivated[NOTE_TYPE.HOLD];
            for(var i = 0; i < array_length(_holds); i++) _holds[i].draw_event(false);
        }
        if(is_array(objEditor.editorNoteAttaching)) {
            for(var i = 0; i < array_length(objEditor.editorNoteAttaching); i++)
                with(objEditor.editorNoteAttaching[i])
                    if(attaching && noteType == NOTE_TYPE.HOLD) draw_event(false);
        }
        dynamiteFxReady = global.noteRenderer.prepare_fx(_playback) > 0;
        if(global.particleEffects == 1) {
            // Ordinary light can shine through a mixer tray. The lamp masks its own stroke.
            if(_trayMask) gpu_set_stencil_enable(false);
            dynamite_draw_shadows();
            if(_trayMask) gpu_set_stencil_enable(true);
        }

        // The original footer has a black backing 66 units tall on its 540-high canvas.
        draw_set_color_alpha(c_black, 127 / 255);
        draw_rectangle(0, _nh - 132, _nw, _nh, false);
        draw_set_alpha(1);
        style_draw_stripe_band(sprDynamiteBottomStripe, _nh - targetLineBelow, _nh,
            sprite_get_height(sprDynamiteBottomStripe) * (100 / 75), 1);
        if(_trayMask) gpu_pop_state();
    }
