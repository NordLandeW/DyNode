
// _position_update();

// Project Stat

projectTime += round(delta_time / 1000);

#region GUI Management
	if(mouse_ishold_r() && !instance_exists(objUISideSwitcher))
		instance_create(mouse_get_last_pos(1)[0], mouse_get_last_pos(1)[1], objUISideSwitcher);
	else if(DEBUG_MODE && mouse_isclick_r()) {
		if(global.__GUIManager == undefined) {
			global.__GUIManager = new GUIManager();
			// var _smh = new Bar("smh", mouse_x+100, mouse_y, "BRUH", 0.5, [0, 100]);
			var _smh = new BarVolumeHitSound("smh", mouse_x+100, mouse_y);
			_smh.set_wh(300, 40);
			var _smh = new StateButton("smh", mouse_x+100, mouse_y+80, "BRUH", true);
			_smh.deactivate();
			var _smh2 = new StateButton("smh2", mouse_x+100, mouse_y+160, "BRUH", false, function (_val) { return _val; });
			var _smh = new ButtonSideSwitcher("smh", mouse_x, mouse_y, 0);
			var _smh2 = new ButtonSideSwitcher("smh2", mouse_x, mouse_y+80, 1);
			var _smh3 = new ButtonSideSwitcher("smh3", mouse_x, mouse_y+160, 2);
			
			// _smh.set_width(100);
		}
		else global.__GUIManager.destroy();
	}
#endregion

#region Functions Control
    
    if(keycheck_down(vk_f3))
        music_load();
    if(keycheck_down(vk_f4))
        background_load();
    if(keycheck_down_ctrl(vk_f4))
    	background_reset();
    if(keycheck_down(vk_f5))
    	map_export_xml(false);
    if(keycheck_down(vk_f6))
    	map_export_xml(true);
    if(keycheck_down(vk_f11))
    	switch_debug_info();
    if(keycheck_down_ctrl(ord("B"))) {
		showBar = !showBar;
		announcement_adjust("anno_show_bar", showBar);
    }
    if(keycheck_down(ord("P"))) {
    	hideScoreboard = !hideScoreboard;
    	announcement_adjust("anno_hide_scoreboard", hideScoreboard);
    }
    if(keycheck_down(ord("O"))) {
    	global.particleEffects = !global.particleEffects;
    	announcement_adjust("anno_particles_effect", global.particleEffects);
    }
    if(keycheck_down_ctrl(ord("H"))) {
    	hitSoundOn = !hitSoundOn;
    	announcement_adjust("anno_hitsound", hitSoundOn);
    }
    
    if(keycheck_down_ctrl(ord("T")))
    	map_set_title();
    if(keycheck_down_ctrl(ord("F"))) {
    	if(editor_get_editside() >= 1 && editor_get_editside() <= 2) {
    		var _side = editor_get_editside() - 1;
    		var _type = chartSideType[_side];
    		
    		switch (_type) {
    			case "MIXER":
    				_type = "MULTI";
    				break;
    			case "MULTI":
    				_type = "PAD";
    				break;
    			default:
    			case "PAD":
    				_type = "MIXER";
    				break;
    		}
    		
	    	chartSideType[_side] = _type;
			dyc_chart_set_sidetype(chartSideType);
	    	announcement_play(i18n_get("anno_switch_sidetype")+chartSideType[_side]);
    	}
    	else {
    		announcement_warning("anno_switch_sidetype_warn");
    	}
    }
    if(keycheck_down(ord("F"))) {
    	fadeOtherNotes = !fadeOtherNotes;
    	announcement_adjust("anno_fade_other_notes", fadeOtherNotes);
    }
    
    if(keycheck_down(vk_enter)) {		// Replay Mode
    	editor_set_editmode(5);
    	nowTime = -PLAYBACK_EMPTY_TIME;
    	animTargetTime = -PLAYBACK_EMPTY_TIME;
    	reset_scoreboard();
    }
    
    if(keycheck_down(ord("U")))
    	map_add_offset("", true);
    
    // Latency Adjust (using key '-' and '=')
    var _map_offset_d = real(keycheck_down(187) - keycheck_down(189));
    if(_map_offset_d!=0)
    	map_add_offset(_map_offset_d * latencyAdjustStep, true);
    var _global_offset_d = real(keycheck_down_ctrl(187) - keycheck_down_ctrl(189));
    if(_global_offset_d!=0)
    	global_add_delay(_global_offset_d * latencyAdjustStep);
    
    
    if(keycheck_down(ord("N"))) {
    	global.simplify = !global.simplify;
    	announcement_adjust("anno_simplify", global.simplify);
    }
    
    if(keycheck_down_ctrl(vk_f6)) {
    	chart_randomize();
    	scribble_anim_wheel(dyc_random_range(15,20), dyc_random_range(9, 20), dyc_random_range(0.5, 5)*global.timeManager.get_fps_scale());
    	announcement_play("[rainbow][wobble][wheel][scale,2]R A N D O M[/rainbow][/wobble][/wheel][/s]\n请谨慎保存谱面。");
    }
    
    if(mouse_check_button_pressed(mb_middle)) {
		stat_next();
    }

	if(keycheck(vk_delete) && keycheck(vk_backspace)) {
		io_clear();
		note_delete_all_manually();
		announcement_play("clear_all_notes");
	}

#endregion
  
#region Scoreboard Update

	var noteCount = dyc_get_note_count();
    if(nowCombo != chartNotesArrayAt && noteCount > 0) {
        var _hit = nowCombo < chartNotesArrayAt;
        if(_hit) {
            with(objPerfectIndc)
                _hitit();
        }
        var _val;
        nowCombo = chartNotesArrayAt;
        _val = 1000000*nowCombo/noteCount;
        with(scbLeft) {
            _update_score(_val, _hit);
        }
        _val = nowCombo;
        with(scbRight) {
            _update_score(_val, _hit, true);
        }
    }
    if(noteCount == 0) {
    	with(scbLeft) _update_score(0, 0);
    	with(scbRight) _update_score(0, 0, true);
    }

#endregion

#region Muisc Pause & Resume

    if(keycheck_down(vk_space) || keycheck_down(vk_enter)) {
    	_set_channel_speed(musicSpeed);
    	if(!nowPlaying || keycheck_down(vk_enter)) {
        	if(nowTime >= musicLength && !keycheck_down(vk_enter)) nowTime = 0;

			// If is recording video, do not resume the music sound.
			if(!global.recordManager.is_recording())
	            FMODGMS_Chan_ResumeChannel(channel);
			
			nowPlaying = true;
            sfmod_channel_set_position(nowTime, channel, sampleRate);

			// Multiple hacks are used for video resume,
			// so there is no need to add safe_video_resume or safe_video_seek_to at here.
        }
        else {
            FMODGMS_Chan_PauseChannel(channel);
            nowPlaying = false;
            
            if(bgVideoLoaded) {
            	safe_video_pause();
            }
        }
    }

#endregion

#region Bg Animation

	standardAlpha = lerp_a(standardAlpha, editor_get_editmode()==5?1:0, animSpeed);
	animTargetBgFaintAlpha = editor_get_editmode() == 5? 0.5: 0;
    bgFaintAlpha = lerp_a(bgFaintAlpha, animTargetBgFaintAlpha, animSpeedFaint);
    
	bgVideoAlpha = lerp(0, safe_video_check_loaded(), standardAlpha);
    
#endregion

#region Targetline Animation

	array_fill(animTargetLineMix, 1, 0, 3);
	if(editor_get_editmode() == 5) {
		array_fill(animTargetLazerAlpha, 1, 0, 3);
	}
	else {
		array_fill(animTargetLazerAlpha, 0, 0, 3);
		for(var i=0; i<3; i++)
			if(editor_editside_allowed(i)) {
				animTargetLazerAlpha[i] = 1.0;
				animTargetLineMix[i] = 0.5;
			}
	}
	
	for(var i=0; i<3; i++) {
		lazerAlpha[i] = lerp_a(lazerAlpha[i], animTargetLazerAlpha[i], animSpeed);
		lineMix[i] = lerp_a(lineMix[i], animTargetLineMix[i], animSpeed);
	}

#endregion

#region Other Animation
	
	animTargetTitleAlpha = editor_get_editmode() == 5? titleAlphaL: 1.0;
	titleAlpha = lerp_a(titleAlpha, animTargetTitleAlpha, animSpeed);
	
#endregion

#region Side Hinter Update

	sideHinterCheckTimer += global.timeManager.get_delta() / 1000;
	if(sideHinterCheckTimer > SIDEHINT_CHECK_TIME) {
		sideHinterCheckTimer = 0;
		_sidehinter_check();
	}
	for(var i=0; i<2; i++) 
		if(sideHinterState[i] >= 0)
		{
			sideHinterTimer[i] += global.timeManager.get_delta() / 1000;
			if(sideHinterTimer[i] >= SIDEHINT_STATE_TIME) {
				sideHinterState[i] ++;
				sideHinterTimer[i] -= SIDEHINT_STATE_TIME;
			}
			if(sideHinterState[i] > 4) {
				sideHinterTimer[i] = 0;
				sideHinterState[i] = -1;
			}
		}

#endregion