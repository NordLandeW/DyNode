
#region Beatlines

	var beatlineVisible = beatlineAlpha[0] + beatlineAlpha[1] + beatlineAlpha[2] > 0.01;
    if(array_length(timingPoints)) {
        var _nw = BASE_RES_W, _nh = BASE_RES_H;
        var nowTime = objMain.nowTime;
        var targetLineBelow = objMain.targetLineBelow + objMain.targetLineBelowH / 2;
        var targetLineBeside = objMain.targetLineBeside;
        var playbackSpeed = objMain.playbackSpeed;
        
        beatlineSurf = surface_checkate(beatlineSurf, _nw, _nh);
        surface_set_target(beatlineSurf);
        draw_clear_alpha(c_black, 0);
        
        // Beat Lines Analyze & Draw
        
        var _nowat = 0, _pointscount = array_length(timingPoints);
        var _totalBeats = 0;
        
        while(_nowat + 1 != _pointscount && timingPoints[_nowat+1].time <= nowTime) {
        	_totalBeats += ceil((timingPoints[_nowat+1].time - timingPoints[_nowat].time)
        		/(timingPoints[_nowat].beatLength*timingPoints[_nowat].meter))
        	_nowat ++;
        }
            
        
        var _nowTp = timingPoints[_nowat];
        var _nowBeats = floor((nowTime - _nowTp.time) / _nowTp.beatLength);
        var _nowTpTime = _nowTp.time;
        var _nextTpTime = (_nowat + 1 == _pointscount ? objMain.musicLength:timingPoints[_nowat+1].time)
        
        var _nowhard = false, _noww, _nowl, _nowh;
        var _ny, _nyl, _nyr;
        
            // Background Glow
            with(objMain) {
                animCurvFaintEval = animcurve_channel_evaluate(
                    animCurvFaintChan, frac(frac((nowTime - _nowTp.time) / _nowTp.beatLength / _nowTp.meter)+1));
                animCurvFaintEval = lerp(0.5, 1.0, animCurvFaintEval);
            }
            
        if(beatlineVisible)
        while(((_nowTpTime - nowTime) * playbackSpeed <= _nh || _nowat == 0) && beatlineAlphaMul > 0.01) {
            for(var i = _nowBeats; i * _nowTp.beatLength + _nowTpTime + 1 < _nextTpTime && (i * _nowTp.beatLength + _nowTpTime - nowTime) * playbackSpeed <= _nh; i++) {
                for(var j = 28; j >= 1; j--) if(j == get_div() || beatlineEnabled[j]) {
                    for(var k = (j == 1? 0:1/j); k < 1 && (i + k) * _nowTp.beatLength + _nowTpTime < _nextTpTime; k += ((j&1)? 1:2)/j) {
                        _ny =  note_time_to_y(_nowTpTime + (i + k) * _nowTp.beatLength, 0);
                        _nyl = note_time_to_y(_nowTpTime + (i + k) * _nowTp.beatLength, 1);
                        _nyr = note_time_to_y(_nowTpTime + (i + k) * _nowTp.beatLength, 2);
                        _nowhard = (k == 0 && i % _nowTp.meter == 0);
                        _noww = _nowhard ? beatlineHardWidth : beatlineWidth;
                        _nowl = _nowhard ? beatlineHardLength : beatlineLength;
                        _nowh = _nowhard ? beatlineHardHeight : beatlineHeight;
                        _noww = _noww * 3;
                        if(j<=28)
                        	_nowl += beatlineLengthOffset[j];
                        if(_ny < 0 && _nyl > _nw / 2)
                            break;
                        
                        var _ncol = j<=28?beatlineColors[j]:0;
                        /// If beatline style is mono
                        if(beatlineStyleCurrent == BeatlineStyles.BS_MONO ||
                           beatlineStyleCurrent == BeatlineStyles.BS_MONOLONG) {
                           		_ncol = _nowhard ? c_white:c_ltgrey;
                           }
                        /// If beatline style is long
                        if(beatlineStyleCurrent == BeatlineStyles.BS_LONG ||
                           beatlineStyleCurrent == BeatlineStyles.BS_MONOLONG) {
                           		_nowl = _nowhard ? beatlineHardLength : beatlineLengthLong;
                           }
                        
                        if(_ncol == 0)
                        	_ncol = c_grey;
                        
                        // draw_set_color(beatlineColors[j]);
                        // LR
                        if(_nyl > targetLineBeside && _nyl <= _nw / 2) {
                            if(beatlineAlpha[1]>0.01) {
                                CleanLine(_nyl, _nh - targetLineBelow - _nowh, _nyl, _nh - targetLineBelow)
                                    .Blend(_ncol, beatlineAlpha[1])
                                    .Thickness(_noww)
                                    .Cap("round", "round")
                                    .Draw();
                                    
                                // draw_set_alpha(beatlineAlpha[1]);
                                // draw_line_width(_nyl, _nh - targetLineBelow - _nowh, _nyl, _nh - targetLineBelow, _noww);
                                
                            }
                                
                            if(beatlineAlpha[2]>0.01) {
                                CleanLine(_nyr, _nh - targetLineBelow - _nowh, _nyr, _nh - targetLineBelow)
                                    .Blend(_ncol, beatlineAlpha[2])
                                    .Thickness(_noww)
                                    .Cap("round", "round")
                                    .Draw();
                                
                                // draw_set_alpha(beatlineAlpha[2]);
                                // draw_line_width(_nyr, _nh - targetLineBelow - _nowh, _nyr, _nh - targetLineBelow, _noww);
                            }
                                
                        }
                        // Down
                        if(_ny <= _nh - targetLineBelow && _ny >= 0 && beatlineAlpha[0]>0.01) {
                            CleanLine(_nw / 2 - _nowl / 2, _ny, _nw / 2 + _nowl / 2, _ny)
                                .Blend(_ncol, beatlineAlpha[0])
                                .Thickness(_noww)
                                .Cap("round", "round")
                                .Draw();
                            
                            // draw_set_alpha(beatlineAlpha[0]);
                            // draw_line_width(_nw / 2 - _nowl / 2, _ny, _nw / 2 + _nowl / 2, _ny, _noww);
                            
                            if(i == 0 && k == 0) {
                                scribble("BPM "+string_format(mspb_to_bpm(_nowTp.beatLength), 1, 2)+" "+string(_nowTp.meter)+"/4")
                                    .starting_format("mDynamix", c_white)
                                    .msdf_border(c_dkgrey, 2)
                                    .align(fa_center, fa_top)
                                    .scale(0.9, 0.9)
                                    .blend(c_white, beatlineAlpha[0])
                                    .draw(_nw/2, _ny+3);
                            }
                            
                            if(_nowhard) {
                            	scribble(string_format(_totalBeats + round(i/_nowTp.meter) + 1, 1, 0))
                            		.align(fa_left, fa_center)
                            		.starting_format("mDynamix", c_white)
                            		.msdf_border(c_dkgrey, 2)
                            		.scale(0.9, 0.9)
                            		.blend(c_white, beatlineAlpha[0])
                            		.draw(beatlineSideInfoX + 20, _ny);
                            }
                            else if(j==1) {
                            	scribble(string_format(_totalBeats + floor(i/_nowTp.meter) + 1, 1, 0))
                            		.align(fa_left, fa_bottom)
                            		.starting_format("mDynamix", c_ltgrey)
                            		.msdf_border(c_dkgrey, 1)
                            		.scale(0.75, 0.75)
                            		.blend(c_white, beatlineAlpha[0])
                            		.draw(beatlineSideInfoX + 10, _ny - 3);
                            		
                            	CleanLine(beatlineSideInfoX, _ny, beatlineSideInfoX+beatlineSideInfoDivWidth, _ny)
                            		.Blend(c_ltgrey, beatlineAlpha[0])
                            		.Thickness(7)
                            		.Cap("round", "round")
                            		.Draw();
                            	
                            	scribble(string_format(i - floor(i/_nowTp.meter)*_nowTp.meter, 1, 0)+"/4")
                            		.align(fa_left, fa_top)
                            		.starting_format("mDynamix", c_ltgrey)
                            		.msdf_border(c_dkgrey, 1)
                            		.scale(0.75, 0.75)
                            		.blend(c_white, beatlineAlpha[0])
                            		.draw(beatlineSideInfoX + 10, _ny + 3);
                            }
                        }
                    }
                    
                    if(j > 28)
                    	j = 29;
                }
            }
            _totalBeats += ceil((_nextTpTime - _nowTpTime) / (_nowTp.beatLength * _nowTp.meter));
            _nowat ++;
            if(_nowat == _pointscount) break;
            _nowTpTime = _nextTpTime;
            _nowTp = timingPoints[_nowat];
            _nextTpTime = (_nowat + 1 == _pointscount ? objMain.musicLength:timingPoints[_nowat+1].time);
            _nowBeats = 0;
        }
        surface_reset_target();
        draw_set_alpha(1.0);
    }

#endregion

// Draw Beatlines
if(surface_exists(beatlineSurf))
    draw_surface_ext(beatlineSurf, 0, 0, 1, 1, 0, c_white, beatlineAlphaMul);

// Draw Highlight Lines

if(editorHighlightLine) {
    draw_set_color_alpha(0x75e7dc, 1);
    var _ny = note_time_to_y(editorHighlightTime, 0),
        _nx = note_time_to_y(editorHighlightTime, 1);
    // Down
    CleanPolyline([30, _ny, BASE_RES_W/2, _ny, BASE_RES_W - 30, _ny])
        .BlendExt([highlightLineColorDownA, 1, highlightLineColorDownB, 0.8, highlightLineColorDownA, 1])
        .Thickness(5)
        .Cap("round", "round")
        .Draw();
    
    // LR
    CleanLine(_nx, 30, _nx, BASE_RES_H - objMain.targetLineBelow - objMain.targetLineBelowH / 2)
        .Thickness(10)
        .Blend2(highlightLineColorSideA, 1,
            highlightLineColorSideB, 1)
        .Cap("round", "none")
        .Draw();
    _nx = BASE_RES_W - _nx;
    CleanLine(_nx, 30, _nx, BASE_RES_H - objMain.targetLineBelow - objMain.targetLineBelowH / 2)
        .Thickness(10)
        .Blend2(highlightLineColorSideA, 1,
            highlightLineColorSideB, 1)
        .Cap("round", "none")
        .Draw();
    
    // Vertical
    draw_set_color_alpha(c_white, 1);
    _nx = note_pos_to_x(editorHighlightPosition, 0);
    if(editorHighlightSide == 0)
        draw_line_width(
        _nx, 0, _nx,
        BASE_RES_H - objMain.targetLineBelow - objMain.targetLineBelowH / 2, 3);
}

// Draw Selction Area
if(editorSelectArea) {
    var _pos = editor_select_get_area_position();
    _pos[0] = clamp(_pos[0], -10, BASE_RES_W+10);
    _pos[2] = clamp(_pos[2], -10, BASE_RES_W+10);
    _pos[1] = clamp(_pos[1], -10, BASE_RES_H+10);
    _pos[3] = clamp(_pos[3], -10, BASE_RES_H+10);
    CleanRectangle(min(_pos[0], _pos[2]), min(_pos[1], _pos[3]), max(_pos[0], _pos[2]), max(_pos[1], _pos[3]))
        .Blend(c_white, 0.2)
        .Border(5, c_white, 0.8)
        .Rounding(4)
        .Draw();
}