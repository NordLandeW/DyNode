
#macro PLAYBACK_EMPTY_TIME (1000)

depth = 0;

// Make Original Background Layer Invisible

    layer_set_visible(layer_get_id("Background"), false);

// FMODGMS Related

    sampleRate = 0;
    channel = undefined;
    music = undefined;
    channelPaused = false;		// Only used for time correction
    musicLength = 0;
    usingMP3 = false;			// For Latency Workaround
    usingPitchShift = false;

#region Time Sources
	
	timesourceSyncVideo = 
		time_source_create(time_source_game, 0.1,
		time_source_units_seconds, function() {
			if(dyc_video_is_loaded()) {
	        	dyc_video_seek_to(nowTime / 1000);
	        }
		}, [], 1);
	
#endregion

#region Layouts Vars

// Target Line

    targetLineBelow = 137;
    targetLineBeside = 112;
    targetLineBelowH = 6;
    targetLineBesideW = 4;
    
    function _position_update() {
        targetLineBelow = 137;
        targetLineBeside = 112;
    }

// Top Progress Bar

    topBarH = 5;
    topBarMouseH = 20;
    topBarMouseInbound = false;
    topBarMousePressed = false;
    topBarMouseLastX = 0;
    topBarIndicatorA = 0;
    animTargetTopBarIndicatorA = 0;
    topBarTimeA = 0;
    animTargetTopBarTimeA = 0;
    topBarTimeLastTime = 0;
    topBarTimeGradA = 0;
    animTargetTopBarTimeGradA = 0;
    
    blackBarFromTop = resor_to_y(125/1080);
    blackBarHeight = 150;
    pauseBarIndent = 40;

#endregion

#region Mixer
    
    #macro MIXER_AVERAGE_TIME_THRESHOLD 10 // ms
    
    mixerX = array_create(2, BASE_RES_H/2);
    mixerNextX = array_create(2, note_pos_to_x(2.5, 1));
    mixerSpeed = 0.5;
    mixerMaxSpeed = 250; // px per frame
    fxClock = 0; // Independent one-shot clock in editor mode (milliseconds).
    mixerShadow = [];
    mixerShadow[0] = instance_create(0, 0, objShadowMIX);
    mixerShadow[1] = instance_create(0, 0, objShadowMIX);
    mixerShadow[0].image_angle = 270;
    mixerShadow[1].image_angle = 90;

    /// @description Return struct with x and time.
    function mixer_get_next_x(side) {
    	var found = false, beginTime = 0, result = 0, accum = 0;
        var _laneSpeed = note_lane_speed(side);
        for(var i=chartNotesArrayAt, l=dyc_get_note_count(); i<l; i++) {
            var _note = dyc_get_note_at_index(i);
            if((_note.time - nowTime) * _laneSpeed / BASE_RES_W > MIXER_REACTION_RANGE)
                break;
        	if(_note.side == side) {
        		if(!found) {
        			found = true;
        			beginTime = _note.time;
        		}
        		if(_note.time - beginTime > MIXER_AVERAGE_TIME_THRESHOLD)
        			break;
        		result += _note.position * (1 / _note.width);
        		accum += 1/_note.width;
        	}
        }
        if(!found) return undefined;
        result /= accum;
        return [note_pos_to_x(result, side), beginTime];
    }

#endregion

#region Side Hinters

#macro SIDEHINT_STATE_TIME (120)        // ms
#macro SIDEHINT_AHEAD_TIME (2600)       // ms
#macro SIDEHINT_SEARCH_TIME (1500)      // ms
#macro SIDEHINT_CHECK_TIME (100)        // ms
    sideLastHitTime = [-1, -1];
    // -1: none; 0,2,4: black; 1,3: white;
    sideHinterState = [-1, -1];
    sideHinterTimer = [0, 0];
    sideHinterCheckTimer = 0;

    function _sidehinter_check() {
        if(!(editor_get_editmode() == 5 && nowPlaying && !topBarMousePressed)) {
            sideLastHitTime[0] = -1;
            sideLastHitTime[1] = -1;
            return;
        }

        for(var i=0; i<2; i++) {
            if(sideLastHitTime[i] < 0)
                sideLastHitTime[i] = nowTime;
            if(nowTime < sideLastHitTime[i]) continue;
            if(sideHinterState[i] == -1)
            {
                var _noteIndex = dyc_get_note_index_on_side_after_index(
                    i + 1,
                    chartNotesArrayAt,
                    nowTime + SIDEHINT_SEARCH_TIME);
                if(_noteIndex >= 0) {
                    var _noteTime = dyc_get_note_time_at_index(_noteIndex);
                    if(_noteTime - sideLastHitTime[i] >= SIDEHINT_AHEAD_TIME) {
                        sideHinterState[i] = 0;
                        sideHinterTimer[i] = 0;
                        sideLastHitTime[i] = _noteTime;
                    }
                }
            }
        }
    }
    function _sidehinter_hit(side, time) {
        sideLastHitTime[side] = time;
    }

#endregion

#region Chart Vars

    chartTitle = "Last Train at 25 O'Clock"
    chartDifficulty = 0;
    chartSideType = ["MIXER", "MULTI"];
    chartID = "";
    chartMusicFile = "";
    chartFile = "";
    
    /// @type {Array<Array<Id.Instance.objNote>>} Activated notes' inst in a step.
    chartNotesArrayActivated = [[], [], []];
    chartNotesArrayAt = 0;
    // Prepared in Draw Begin, shared by the shadow and foreground layers.
    dynamiteNoteBound = 0;
    dynamiteFxReady = false;

#endregion

#region Playview Properties

    themeColor = theme_get().color;

	timeBoundLimit = 3000; // Time bound before the musics starts
    nowBar = 0;
    nowTime = 0;
    nowPlaying = false;
    nowScore = 0;
    nowCombo = 0;
    playbackSpeed = 1.6;
    sideSpeedScale = style_side_speed_scale();
    adtimeSpeed = 50.0; // Use AD to Adjust Time ms per frame
    scrolltimeSpeed = 120.0; // Use mouse scroll to Adjust Time ms per frame
    
    animSpeed = 0.3;
    animTargetTime = 0;
    animTargetPlaybackSpeed = playbackSpeed;
    
    musicProgress = 0.0;
    musicSpeed = 1.0;
    musicResyncRequest = false;
    
    hideScoreboard = true;			// hide score board under editor mode
    hitSoundOn = false;

    volumeMain = 1.0;           // Music sound volume
    volumeHit = 1.0;            // Hit sound volume
    
    showDebugInfo = DEBUG_MODE ? 1 : 0;
    showStats = 0;
    showBar = false;
    fadeOtherNotes = false;
    
    statCount = undefined;
    stat_reset();
    
    // For 3 sides targetline's glow
    lazerAlpha = [1.0, 1.0, 1.0];
    animTargetLazerAlpha = [1.0, 1.0, 1.0];
    lineMix = [1.0, 1.0, 1.0];
    animTargetLineMix = [1.0, 1.0, 1.0];
    titleAlphaL = 0.5;
    titleAlpha = titleAlphaL;
    animTargetTitleAlpha = titleAlphaL;
    
    standardAlpha = 0; // For editmode switch usage
    autoBadgeClearAlpha = 1;

    // Stat vars
    statKPS = 0;
    
    // Bottom
        bottomDim = 0.6;
    
    // Background
        bgDim = 0.65;
        
        // Image
        bgImageSpr = -1;
        
        // Video
        bgVideoAlpha = 0;
        
        // Faint
        bgFaintAlpha = 1;
        animCurvFaintChan = animcurve_get_channel(curvBgGlow, "curve1");
        animCurvFaintEval = 0.5;
        animTargetBgFaintAlpha = 0.5; 
        animSpeedFaint = 0.1;
        
        function _faint_hit() {
            bgFaintAlpha = 0.7;
        }

#endregion

#region Surfaces

    judgmentHudSurf = -1;
    dynamiteShadowSurf = -1;
    shadowPingSurf = -999;
    shadowPongSurf = -999;
    partSurf = -999;

#endregion

#region Particles Init

    // PartSys
    partSysNote = part_system_create();
    partAlphaMul = 0.75;
    part_system_automatic_draw(partSysNote, false);

    // PartType
    
        // Note
        function _parttype_noted_init(_pt, _scl = 1.0, _ang = 0.0, _mixer = false) {
            var fpsAdjust = global.timeManager.get_fps_scale();
            var _fps = global.timeManager.get_fps();
        	var _theme = theme_get();
            part_type_sprite(_pt, _theme.partSpr, false, true, false);
            if(_theme.partBlend)
            	part_type_alpha3(_pt, partAlphaMul, 0.6 * partAlphaMul, 0);
            else
            	part_type_alpha3(_pt, 1, 1, 0);
            
            if(_mixer) {
                part_type_speed(_pt, _scl * 15 * fpsAdjust, _scl * 15 * fpsAdjust, 0, 0);
                part_type_life(_pt, _fps*0.23, _fps*0.25);
                part_type_size(_pt, 0.7, 0.9, -0.04 * fpsAdjust, 0);
            }
            else {
                part_type_speed(_pt, _scl * 23 * fpsAdjust, _scl * 23 * fpsAdjust, 0, 0);
                part_type_life(_pt, _fps*0.2, _fps*0.4);
                part_type_size(_pt, 1.0, 1.2, -0.03 * fpsAdjust, -0.04 * fpsAdjust);
            }
            part_type_color3(_pt, c_white, _theme.partColA, _theme.partColB);
            // part_type_color2(_pt, 0x652dba, themeColor);
            // part_type_color2(_pt, _theme.partColA, _theme.partColB);
            part_type_scale(_pt, _scl * 2, _scl * 2);
            part_type_orientation(_pt, 0, 360, 2 * fpsAdjust, 0, false);
            part_type_blend(_pt, _theme.partBlend);
            part_type_direction(_pt, _ang, _ang, 0, 0);
        }
        
        partTypeNoteDL = part_type_create();
        
        partTypeNoteDR = part_type_create();
        
        
        // Hold
        function _parttype_hold_init(_pt, _scl = 1.0, _ang = 0.0) {
        	var _theme = theme_get();
            var fpsAdjust = global.timeManager.get_fps_scale();
            var _fps = global.timeManager.get_fps();
            part_type_sprite(_pt, _theme.partSpr, false, true, false);
            part_type_alpha3(_pt, 1, 1, 0);
            part_type_speed(_pt, _scl * 8 * fpsAdjust, _scl * 12 * fpsAdjust, 0, 0);
            part_type_color2(_pt, _theme.partColHA, _theme.partColHB);
            // part_type_color2(_pt, 0x89ffff, 0xffffe5)
            part_type_size(_pt, 1.2, 1.4, -0.02 * fpsAdjust, -0.02 * fpsAdjust);
            // part_type_scale(_pt, _scl * 2, _scl * 2);
            part_type_orientation(_pt, 0, 360, 1 * fpsAdjust, 1 * fpsAdjust, false);
            part_type_life(_pt, _fps*0.4, _fps*0.5);
            part_type_blend(_pt, true);
            part_type_direction(_pt, 0, 360, 0, 0);
        }
        partTypeHold = part_type_create();
        
        function _partsys_init() {
        	_parttype_noted_init(partTypeNoteDL);
        	_parttype_noted_init(partTypeNoteDR, 1, 180);
        	_parttype_hold_init(partTypeHold);
        }
        _partsys_init();
        
    // Part Emitter
    
        partEmit = part_emitter_create(partSysNote);
        function _partemit_init(_pe, _x1, _y1, _x2, _y2) {
            part_emitter_region(partSysNote, _pe, _x1, _y1, _x2, _y2, 
                ps_shape_line, ps_distr_linear);
        }

#endregion



#region Scoreboard Init

    scbDepth = 1000;
    scbLeft = create_scoreboard(resor_to_x(295/1920), resor_to_y(555/1080),
        scbDepth, 7, fa_middle, 0);
    scbRight = create_scoreboard(resor_to_x(1-295/1920), resor_to_y(555/1080),
        scbDepth, 0, fa_right, 3);

#endregion

#region Perfect Indicator Init

    perfDepth = 1000;
    perfLeft = instance_create_depth(resor_to_x(295/1920), resor_to_y(636/1080), 
        perfDepth, objPerfectIndc);
    perfRight = instance_create_depth(resor_to_x(1206/1920), resor_to_y(636/1080), 
        perfDepth, objPerfectIndc);
        
#endregion

#region Editor Init

    editor = instance_create_depth(0, 0, -10000, objEditor);

#endregion

#region TopBar Init

	topbar = instance_create_depth(0, 0, 0, objTopBar);

#endregion
    
// Tool Related

	latencyAdjustStep = 5;	// in ms

// Project stats

    projectTime = 0;    // in ms

#region Methods

// Set music's time to [time].
// If not [animated], there will be no transition animation when time is set.
// If [inbound!=-1], the time being set will stay above targetline in [inbound] pixels
function time_set(time, animated = true, inbound = -1) {
	if(inbound > 0) {
		// If above screen
		if(time > nowTime) 
			time -= pix_to_note_time(BASE_RES_H - targetLineBelow - inbound);
		else
			time -= pix_to_note_time(inbound);
	}
		
	animTargetTime = time;
	if(!animated || nowPlaying)
		nowTime = time;
	if(nowPlaying)
		time_music_sync();
}

// Get music's time.
// If [animated], the precise time displayed on screen (including transition animation) will be returned.
function time_get(animated = false) {
	return animated?animTargetTime:nowTime;
}

// Add music's time with [offset].
function time_add(offset, animated = true) {
	time_set(time_get() + offset, animated);
}

// Check if given [time] is inbound.
function time_inbound(time) {
	return time>nowTime && note_time_to_pix(time - nowTime) + targetLineBelow < BASE_RES_H;
}

// Make the specific time range inbound.
function time_range_made_inbound(timeL, timeR, inbound = 300, animated = true) {
	var _il = time_inbound(timeL);
	var _ir = time_inbound(timeR);
	if(!_il && !_ir) {
		if(in_between(nowTime, timeL, timeR)) return;
		else if(nowTime > timeR)
			time_set(timeR, animated, inbound);
		else
			time_set(timeL, animated, inbound);
	}
}

// Sync the time with music.
function time_music_sync() {
	if(!nowPlaying)
		return;
	
	// Set the fmod channel
	sfmod_channel_set_position(nowTime, channel, sampleRate);
    
    // Sync with the video position
    if(dyc_video_is_loaded()) {
    	time_source_start(timesourceSyncVideo);
    }
}

// Switch whether channel using Pitch Shift effect.
function music_pitchshift_switch(enable) {
	if(enable != usingPitchShift) {
		usingPitchShift = enable;

        // Enable the effect in method below.
        _set_channel_speed(musicSpeed);
	}
}

function volume_get_hitsound() {
	if(!objMain.hitSoundOn)
		return 0;
    return volumeHit;
}

function volume_set_hitsound(_vol) {
	if(_vol == 0) {
    	objMain.hitSoundOn = false;
    }
    else {
    	objMain.hitSoundOn = true;
    	audio_sound_gain(sndHit, _vol, 0);
        volumeHit = _vol;
    }
}

function volume_get_main() {
	if(music==undefined) return 0;
	return volumeMain;
}

function volume_set_main(_vol) {
	volumeMain = _vol;
    FMODGMS_Chan_Set_Volume(channel, _vol);
}

function _create_channel() {
	FMODGMS_Chan_RemoveChannel(channel);
	channel = FMODGMS_Chan_CreateChannel();
}

function _set_channel_speed(spd) {
    FMODGMS_Chan_Set_Pitch(channel, spd);
    FMODGMS_Chan_Set_Volume(channel, volumeMain);
	if(objMain.usingPitchShift && spd != 1) {
		FMODGMS_Effect_Set_Parameter(global.__DSP_Effect, FMOD_DSP_PITCHSHIFT.FMOD_DSP_PITCHSHIFT_PITCH, 1.0/spd);
		FMODGMS_Chan_Remove_Effect(channel, global.__DSP_Effect);
		FMODGMS_Chan_Add_Effect(channel, global.__DSP_Effect, 1);
	}
    else {
    	FMODGMS_Chan_Remove_Effect(channel, global.__DSP_Effect);
    }
	
    dyc_video_set_speed(spd);
    musicResyncRequest = true;
}

function _reset_all_particles() {
    part_particles_clear(partSysNote);
}

function pitchshift_effect_enabled() {
    return usingPitchShift && musicSpeed != 1;
}

/// @description Begin an isolated, premultiplied HUD layer in playfield coordinates.
function judgment_hud_begin() {
    judgmentHudSurf = surface_check(judgmentHudSurf, view_wport[0], view_hport[0]);
    if(!surface_exists(judgmentHudSurf)) return false;
    surface_set_target(judgmentHudSurf);
    manually_set_view_size(BASE_RES_W, BASE_RES_H);
    gpu_push_state();
    gpu_set_alphatestenable(false);
    draw_clear_alpha(c_black, 0);
    return true;
}

/// @description Composite a premultiplied layer with the existing shadow theme treatment.
function shadow_layer_draw(_surface) {
    gpu_push_state();
    if(global.themeAt == 2) {
        shader_set(shd_mono);
        gpu_set_blendmode(bm_add);
    }
    else {
        gpu_set_blendmode_ext(bm_one, bm_inv_src_alpha);
        if(global.themeAt > 0) {
            shader_set(shd_hsv_trans);
            var _hsv = theme_get_color_hsv();
            shader_set_uniform_q("u_hue", _hsv[0]);
            shader_set_uniform_q("u_saturation", _hsv[1]);
        }
    }
    draw_surface_stretched_ext(_surface, 0, 0, BASE_RES_W, BASE_RES_H, c_white, 1);
    if(global.themeAt > 0) shader_reset();
    gpu_pop_state();
}

/// @description Restore the playfield target and composite the judgment HUD once.
function judgment_hud_end(_shadowTheme) {
    manually_reset_view_size();
    surface_reset_target();
    if(_shadowTheme) {
        shadow_layer_draw(judgmentHudSurf);
    }
    else {
        gpu_set_blendmode_ext(bm_one, bm_inv_src_alpha);
        draw_surface_stretched_ext(judgmentHudSurf, 0, 0, BASE_RES_W, BASE_RES_H, c_white, 1);
    }
    gpu_pop_state();
}

/// @description Keep Dynamix mixer lights below the notes, independent of normal note shadows.
function dynamix_draw_mixer_lights() {
    var _active = false;
    for(var i = 0; i < 2; i++) {
        var _light = mixerShadow[i];
        if(chartSideType[i] != "MIXER" || _light.image_alpha <= 0) continue;
        if(!_active) {
            if(!judgment_hud_begin()) return;
            gpu_set_blendmode_ext_sepalpha(bm_src_alpha, bm_inv_src_alpha, bm_one, bm_inv_src_alpha);
            _active = true;
        }
        draw_sprite_ext(_light.sprite_index, 0, _light.x, _light.y, _light.image_xscale, _light.image_yscale,
                        _light.image_angle, _light.image_blend, _light.image_alpha * (global.themeAt == 2 ? 1 : SHADOW_LAYER_ALPHA));
    }
    if(_active) judgment_hud_end(true);
}

#macro DYNAMITE_TRAY_DX (0.32 * BASE_RES_H / 16)
#macro DYNAMITE_TRAY_DY (1.6 * BASE_RES_H / 16)
#macro DYNAMITE_TRAY_HALF_STROKE (0.06 * BASE_RES_H / 16)
#macro DYNAMITE_TRAY_OUTER_DX (DYNAMITE_TRAY_DX + DYNAMITE_TRAY_HALF_STROKE)
#macro DYNAMITE_TRAY_OUTER_DY (DYNAMITE_TRAY_DY + DYNAMITE_TRAY_HALF_STROKE)

/// @description Clip subsequent HUD artwork and notes to the outside of each mixer's outer stroke.
function dynamite_begin_tray_mask() {
    if(chartSideType[0] != "MIXER" && chartSideType[1] != "MIXER") return false;

    gpu_push_state();
    gpu_set_stencil_enable(true);
    draw_clear_stencil(0);
    gpu_push_state();
    gpu_set_colorwriteenable(false, false, false, false);
    gpu_set_zwriteenable(false);
    gpu_set_ztestenable(false);
    gpu_set_alphatestenable(false);
    gpu_set_stencil_ref(1);
    gpu_set_stencil_func(cmpfunc_always);
    gpu_set_stencil_pass(stencilop_replace);
    var _dx = DYNAMITE_TRAY_OUTER_DX, _dy = DYNAMITE_TRAY_OUTER_DY;
    for(var i = 0; i < 2; i++) {
        if(chartSideType[i] == "MIXER") {
            var _x = i == 0 ? targetLineBeside : BASE_RES_W - targetLineBeside;
            draw_rectangle(_x - _dx, mixerX[i] - _dy, _x + _dx, mixerX[i] + _dy, false);
        }
    }
    gpu_pop_state();
    gpu_set_stencil_ref(0);
    gpu_set_stencil_func(cmpfunc_equal);
    gpu_set_stencil_pass(stencilop_keep);
    return true;
}

/// @description Add the emitted light to the dimmed playfield without changing its coverage.
function dynamite_draw_shadows() {
    var _renderer = global.noteRenderer;
    var _size = dynamiteFxReady ? _renderer.buffer_fx(DYNAMITE_FX_PASS.SHADOW) : 0;
    var _leftAge = chartSideType[0] == "MIXER" ? DyCore_dynamite_mixer_age(NOTE_SIDE.LEFT) : -1;
    var _rightAge = chartSideType[1] == "MIXER" ? DyCore_dynamite_mixer_age(NOTE_SIDE.RIGHT) : -1;
    // The original mixer lamp loses .04 alpha per update at its reference 120 Hz.
    var _left = _leftAge < 0 ? 0 : max(0, 1 - _leftAge * 0.0048);
    var _right = _rightAge < 0 ? 0 : max(0, 1 - _rightAge * 0.0048);
    if(_size <= 0 && _left <= 0 && _right <= 0) return;

    dynamiteShadowSurf = surface_check(dynamiteShadowSurf, view_wport[0], view_hport[0]);
    if(!surface_exists(dynamiteShadowSurf)) return;
    surface_set_target(dynamiteShadowSurf);
    manually_set_view_size(BASE_RES_W, BASE_RES_H);
    gpu_push_state();
    gpu_set_alphatestenable(false);
    draw_clear_alpha(c_black, 0);

    for(var i = 0; i < 2; i++) {
        var _alpha = i == 0 ? _left : _right;
        if(_alpha <= 0) continue;
        var _x = i == 0 ? targetLineBeside : BASE_RES_W - targetLineBeside;
        var _y = mixerX[i], _scale = DYNAMITE_TRAY_DY / 100;
        gpu_set_blendmode_ext_sepalpha(bm_src_alpha, bm_one, bm_zero, bm_one);
        draw_sprite_ext(sprDynamiteMixerLight, 0, _x, _y, _scale, _scale, i == 0 ? 180 : 0, c_white, _alpha);
        // Remove this lamp through the outer edge of the tray, including its translucent stroke.
        gpu_set_blendmode_ext(bm_zero, bm_zero);
        draw_rectangle(_x - DYNAMITE_TRAY_OUTER_DX, _y - DYNAMITE_TRAY_OUTER_DY,
                       _x + DYNAMITE_TRAY_OUTER_DX, _y + DYNAMITE_TRAY_OUTER_DY, false);
    }
    if(_size > 0) _renderer.draw_fx(DYNAMITE_FX_PASS.SHADOW, _size);
    manually_reset_view_size();
    surface_reset_target();
    // RGB already includes source alpha. Add it exactly once, keeping destination alpha.
    gpu_set_blendmode_ext_sepalpha(bm_one, bm_one, bm_zero, bm_one);
    draw_surface_stretched_ext(dynamiteShadowSurf, 0, 0, BASE_RES_W, BASE_RES_H, c_white, 1);
    gpu_pop_state();
}

/// @description Draw the lane lines and hollow mixer trays without masking the playfield.
function dynamite_draw_judgment_hud() {
    var _halfLine = DYNAMITE_TRAY_HALF_STROKE, _frontY = BASE_RES_H - targetLineBelow;
    var _playback = editor_get_editmode() == 5;
    var _mask = chartSideType[0] == "MIXER" || chartSideType[1] == "MIXER";
    if(_mask) {
        if(!judgment_hud_begin()) return;
        // Replace the masked line pixels, including their alpha.
        gpu_set_blendmode_ext_sepalpha(bm_src_alpha, bm_zero, bm_one, bm_zero);
    }

    var _frontTop = _frontY - _halfLine;
    static _sideAlpha = [1, 1];
    for(var i = 0; i < 2; i++) {
        var _alpha = _playback ? 1 : lerp(0.5, 1, lazerAlpha[NOTE_SIDE.LEFT + i]);
        if(_playback && nowPlaying) {
            var _hint = sideHinterState[i];
            if(_hint >= 0 && _hint % 2 == 0) _alpha = 0.3;
        }
        _sideAlpha[i] = _alpha;
        draw_set_color_alpha(c_white, _alpha);
        var _x = i == 0 ? targetLineBeside : BASE_RES_W - targetLineBeside;
        draw_rectangle(_x - _halfLine, 0, _x + _halfLine, _frontTop, false);
    }

    // Share the edge so translucent lines meet without blending twice on the playfield.
    draw_set_alpha(_playback ? 1 : lerp(0.5, 1, lazerAlpha[NOTE_SIDE.FRONT]));
    draw_rectangle(0, _frontTop, BASE_RES_W, _frontY + _halfLine, false);

    if(_mask) {
        for(var i = 0; i < 2; i++) {
            if(chartSideType[i] != "MIXER") continue;
            var _x = i == 0 ? targetLineBeside : BASE_RES_W - targetLineBeside;
            var _y = mixerX[i];
            var _dx = DYNAMITE_TRAY_DX, _dy = DYNAMITE_TRAY_DY;
            var _l = _x - _dx, _r = _x + _dx, _t = _y - _dy, _b = _y + _dy;
            // End the cut beneath the frame stroke so fractional positions cannot open a seam.
            draw_set_alpha(0);
            draw_rectangle(_l, _t, _r, _b, false);
            draw_set_alpha(_sideAlpha[i]);
            draw_rectangle(_l - _halfLine, _t - _halfLine, _r + _halfLine, _t + _halfLine, false);
            draw_rectangle(_l - _halfLine, _b - _halfLine, _r + _halfLine, _b + _halfLine, false);
            draw_rectangle(_l - _halfLine, _t + _halfLine, _l + _halfLine, _b - _halfLine, false);
            draw_rectangle(_r - _halfLine, _t + _halfLine, _r + _halfLine, _b - _halfLine, false);
        }
    }
    draw_set_alpha(1);
    if(_mask) judgment_hud_end(false);
}

/// @description Draw the Dynamite chart title and mode badges below the front judgment line.
function dynamite_draw_footer() {
    var _lineY = BASE_RES_H - targetLineBelow;
    var _oldFont = draw_get_font();
    var _oldHAlign = draw_get_halign(), _oldVAlign = draw_get_valign();
    var _oldColor = draw_get_color(), _oldAlpha = draw_get_alpha();

    draw_set_color_alpha(c_white, 1);
    draw_set_valign(fa_top);
    style_draw_chart_title(chartTitle, 45, _lineY + 11, 1);

    // Frames 0..5 follow the chart difficulty order; frame 6 is the AUTO badge.
    draw_sprite_ext(sprDynamiteBadges, chartDifficulty, 42, _lineY + 72, 1, 1, 0, c_white, 1);
    draw_set_halign(fa_center);
    draw_set_font(fDynamiteHeavy);
    draw_text(142, _lineY + 73, difficulty_num_to_name(chartDifficulty));
    var _autoAlpha = standardAlpha * autoBadgeClearAlpha;
    if(_autoAlpha > 0) {
        var _autoX = BASE_RES_W - 228;
        draw_sprite_ext(sprDynamiteBadges, 6, _autoX, _lineY + 68, 1, 1, 0, c_white, _autoAlpha);
        draw_set_alpha(_autoAlpha);
        draw_set_font(fDynamiteLight);
        // The original Light atlas uses 48px glyphs; this label is 32px on the design canvas.
        draw_text_transformed(_autoX + 100, _lineY + 69, "AUTO", 2 / 3, 2 / 3, 0);
    }

    draw_set_font(_oldFont);
    draw_set_halign(_oldHAlign);
    draw_set_valign(_oldVAlign);
    draw_set_color_alpha(_oldColor, _oldAlpha);
}

#endregion

_create_channel();	// Channel init.
