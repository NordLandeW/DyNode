
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
    mixerShadow = [];
    mixerShadow[0] = instance_create(0, 0, objShadowMIX);
    mixerShadow[1] = instance_create(0, 0, objShadowMIX);
    mixerShadow[0].image_angle = 270;
    mixerShadow[1].image_angle = 90;

    /// @description Return struct with x and time.
    function mixer_get_next_x(side) {
    	var found = false, beginTime = 0, result = 0, accum = 0;
        for(var i=chartNotesArrayAt, l=dyc_get_note_count(); i<l; i++) {
            var _note = dyc_get_note_at_index(i);
            if((_note.time - nowTime) * playbackSpeed / BASE_RES_W > MIXER_REACTION_RANGE)
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
                for(var j=chartNotesArrayAt, l=dyc_get_note_count(); j<l; j++) {
                    var _note = dyc_get_note_at_index(j);
                    if(_note.time - nowTime > SIDEHINT_SEARCH_TIME) break;
                    if(_note.side == i + 1) {
                        if(_note.time - sideLastHitTime[i] >= SIDEHINT_AHEAD_TIME) {
                            sideHinterState[i] = 0;
                            sideHinterTimer[i] = 0;
                            sideLastHitTime[i] = _note.time;
                        }
                        break;
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
    adtimeSpeed = 50.0; // Use AD to Adjust Time ms per frame
    scrolltimeSpeed = 120.0; // Use mouse scroll to Adjust Time ms per frame
    
    animSpeed = 0.3;
    animTargetTime = 0;
    animTargetPlaybackSpeed = playbackSpeed;
    
    musicProgress = 0.0;
    musicSpeed = 1.0;
    
    hideScoreboard = true;			// hide score board under editor mode
    hitSoundOn = false;

    volumeMain = 1.0;           // Music sound volume
    volumeHit = 1.0;            // Hit sound volume
    
    showDebugInfo = DEBUG_MODE ? 1 : 0;
    showStats = 0;
    showBar = false;
    fadeOtherNotes = false;
    bgBlured = false;
    
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

    // Stat vars
    statKPS = 0;
    
    // Bottom
        bottomDim = 0.6;
        bottomBgBlurIterations = 3;
    
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
        
        // Kawase Blur

		kawaseArr = kawase_create(BASE_RES_W, targetLineBelow, bottomBgBlurIterations);

#endregion

#region Surfaces

    shadowPingSurf = -999;
    shadowPongSurf = -999;

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
		if(channel>=0) {
			if(enable) {
				FMODGMS_Chan_Add_Effect(channel, global.__DSP_Effect, 1);
			}
			else {
				FMODGMS_Chan_Remove_Effect(channel, global.__DSP_Effect);
			}
		}
		
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
	if(USE_DSP_PITCHSHIFT)
    FMODGMS_Chan_Add_Effect(channel, global.__DSP_Effect, 0);
}

function _set_channel_speed(spd) {
    FMODGMS_Chan_Set_Pitch(channel, spd);
    FMODGMS_Chan_Set_Volume(channel, volumeMain);
	if(USE_DSP_PITCHSHIFT) {
		FMODGMS_Effect_Set_Parameter(global.__DSP_Effect, FMOD_DSP_PITCHSHIFT.FMOD_DSP_PITCHSHIFT_PITCH, 1.0/spd);
		FMODGMS_Chan_Remove_Effect(channel, global.__DSP_Effect);
		FMODGMS_Chan_Add_Effect(channel, global.__DSP_Effect, 1);
	}
	
    dyc_video_set_speed(spd);
}

function _reset_all_particles() {
    part_particles_clear(partSysNote);
}

#endregion

_create_channel();	// Channel init.