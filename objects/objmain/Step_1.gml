/// @description Reset Tags & Time Update & Activate notes

#region TIME UPDATE

var _music_resync_request = false;

// Music Speed Adjust
    
    var _muspdchange = keycheck_down(ord("W")) - keycheck_down(ord("S"));
    if(_muspdchange != 0) {
        musicSpeed += 0.1 * _muspdchange;
        musicSpeed = max(musicSpeed, 0.1);
        _set_channel_speed(musicSpeed);
        
        announcement_play(i18n_get("anno_music_speed") + ": x" + string_format(musicSpeed, 1, 1), 3000, "music_speed");
    }

// Keyboard Time & Speed Adjust

    var _spdchange = keycheck_down(ord("E")) - keycheck_down(ord("Q"));
    _spdchange += editor_select_is_going()? 0: wheelcheck_up_ctrl() - ((animTargetPlaybackSpeed > 0.2) * wheelcheck_down_ctrl());
    animTargetPlaybackSpeed += 0.1 * _spdchange;
    
    if(_spdchange != 0) {
    	announcement_play(i18n_get("anno_note_speed") + ": x" + string_format(animTargetPlaybackSpeed, 1, 2), 3000, "anno_note_speed");
    	
    	if(animTargetPlaybackSpeed == 0.2 && _spdchange < 0)
    		announcement_warning("anno_note_speed_warn");
    }
    
    playbackSpeed = lerp_a(playbackSpeed, animTargetPlaybackSpeed, animSpeed);
    
    var _timchange = keycheck(ord("D")) - keycheck(ord("A"));
    var _timscr = wheelcheck_up() - wheelcheck_down();
    _timchange += 3 * (keycheck_shift(ord("D")) - keycheck_shift(ord("A")))
    
    if(_timchange != 0 || _timscr != 0) {
        if(nowPlaying) {
            nowTime += (_timchange * adtimeSpeed * global.fpsAdjust + _timscr * scrolltimeSpeed);
            _music_resync_request = true;
        }
        else {
            animTargetTime += (_timchange * adtimeSpeed * global.fpsAdjust + _timscr * scrolltimeSpeed);
        }
    }
    
    if(nowPlaying && keycheck(vk_enter)) {
    	nowTime = 0;
    	_music_resync_request = true;
    }

// Time Operation

    if(nowPlaying && !(_timchange != 0 || _timscr != 0)) {
    	nowTime += delta_time * musicSpeed / 1000;
    }
    
        
    if(music != undefined) {
        // Play music at chart's beginning
        if(nowTime < 0) {
            FMODGMS_Chan_PauseChannel(channel);
            sfmod_channel_set_position(0, channel, sampleRate);
            channelPaused = true;
        }
        else if(nowPlaying && channelPaused) {
            FMODGMS_Chan_ResumeChannel(channel);
            sfmod_channel_set_position(nowTime, channel, sampleRate);
            channelPaused = false;
        }
        
        // Top Bar Adjust Part
        
    		topBarMouseInbound = (mouse_y <= topBarMouseH && mouse_y >= 0) || alt_ishold();
            topBarMouseInbound = topBarMouseInbound && editor_get_editmode() >= 4;
            if(topBarMouseInbound || topBarMousePressed || _timchange != 0 || _timscr != 0) {
            	animTargetTopBarIndicatorA = 0.3;
            	topBarTimeLastTime = 2000;
            }
            else
                animTargetTopBarIndicatorA = 0;
            
            topBarIndicatorA = lerp(topBarIndicatorA, 
                animTargetTopBarIndicatorA, animSpeed * global.fpsAdjust);
            
            topBarTimeLastTime -= delta_time / 1000;
            if(editor_get_editmode() < 5) topBarTimeLastTime = 1;
        	
            animTargetTopBarTimeGradA = topBarMouseInbound || topBarMousePressed;
        	animTargetTopBarTimeA = topBarTimeLastTime > 0;
        	topBarTimeA = lerp_a(topBarTimeA, animTargetTopBarTimeA, 0.2);
        	topBarTimeGradA = lerp_a(topBarTimeGradA, animTargetTopBarTimeGradA, 0.2);
                
        
            if(mouse_check_button_pressed(mb_left) && topBarMouseInbound) {
                topBarMousePressed = true;
                topBarMouseLastX = -5;
            }
                
            
            if(topBarMousePressed) {
            	mouse_clear_hold(); // Clear the Hold Buffer
                if(mouse_check_button_released(mb_left)) {
                	topBarMousePressed = false;
                	mouse_clear(mb_left);
                }
                    
                else {
                    if(nowPlaying) {
                        if(abs(topBarMouseLastX - mouse_x) >= 2) {
                            musicProgress = mouse_x / global.resolutionW;
                            nowTime = musicProgress * musicLength;
                            _music_resync_request = true;
                        }
                        topBarMouseLastX = mouse_x;
                    }
                    else {
                        musicProgress = mouse_x / global.resolutionW;
                        animTargetTime = musicProgress * musicLength;
                    }
                }
            }
        
        // If music ends then stop
        if((FMODGMS_Chan_Is_Playing(channel)<=0 || nowTime >= musicLength) && nowPlaying) {
        	
            // Channel gets invalid, create another one.
            _create_channel();
            FMODGMS_Snd_PlaySound(music, channel);
            _set_channel_speed(musicSpeed);
            FMODGMS_Chan_PauseChannel(channel);
            nowTime = musicLength;
            animTargetTime = musicLength;
            safe_video_pause();
            
            nowPlaying = false;
        }
        
        musicProgress = clamp(nowTime, 0, musicLength) / musicLength;
        
        animTargetTime = clamp(animTargetTime, -timeBoundLimit, musicLength);
    }
    
    else {
        musicProgress = 0;
    }
    
// Time Jump

    if(keycheck_down_ctrl(ord("L")) && chartNotesArrayAt<chartNotesCount)
        animTargetTime = chartNotesArray[chartNotesArrayAt].time;
    if(keycheck_down_ctrl(ord("K")) && chartNotesArrayAt>0)
        animTargetTime = chartNotesArray[chartNotesArrayAt-1].time;

// Update and Sync Time & musicTime

    if(nowPlaying) {
        if(music != undefined)
            nowTime = clamp(nowTime, -timeBoundLimit, musicLength);
        animTargetTime = nowTime;
    }
    else {
        nowTime = lerp_lim_a(nowTime, animTargetTime, animSpeed, 10000);
        
        if(abs(nowTime - animTargetTime) < 1)
            nowTime = animTargetTime; // Speeeed up
    }
    
    if(_music_resync_request) {
        time_music_sync();
    }

#endregion

#region Chart Properties Update

	// Adjust Difficulty
	var _diff_delta = keycheck_down_ctrl(ord("P")) - keycheck_down_ctrl(ord("O"));
	chartDifficulty += _diff_delta;
	chartDifficulty = clamp(chartDifficulty, 0, global.difficultyCount - 1);

    chartNotesCount = array_length(chartNotesArray)

	chartNotesArrayAt = clamp(chartNotesArrayAt, 0, chartNotesCount);
	
    while(chartNotesArrayAt < chartNotesCount &&
        chartNotesArray[chartNotesArrayAt].time <= nowTime) {
            chartNotesArrayAt ++;
            if(chartNotesArrayAt < chartNotesCount)
                note_check_and_activate(chartNotesArrayAt);
        }

    while(chartNotesArrayAt > 0 &&
        chartNotesArray[chartNotesArrayAt-1].time > nowTime){
            chartNotesArrayAt --;
            note_check_and_activate(chartNotesArrayAt);
        }

#endregion

#region NOTES ACTIVATE & DEACTIVATE

	// if(ds_map_size(deactivationQueue) > NOTE_DEACTIVATION_LIMIT)
	// 	note_deactivate_flush();

	var i=max(chartNotesArrayAt-3, 0), l=chartNotesCount;
	var _editMode = editor_get_editmode();
	for(; i<l; i++) {
        if(_editMode == 5 && note_exists(chartNotesArray[i].inst))
            continue;
		if(note_check_and_activate(i) < 0)
			break;
    }

#endregion