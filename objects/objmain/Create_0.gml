
depth = 0;

// Make Original Background Layer Invisible

    layer_set_visible(layer_get_id("Background"), false);
    
#region Optimization

	deactivationQueue = ds_map_create();
	savingProjectId = {id: -1, buffer: undefined}; // The project being saved async
	savingExportId = {id: -1, buffer: undefined}; // The map being export async
	
#endregion

#region Time Sources
	
	// To prevent unstable delay from music to chart
	var _tsFun = function() {
		nowPlaying = true;
		nowTime = sfmod_channel_get_position(channel, sampleRate);
	};
	resumeDelay = 15;
	timesourceResumeDelay =
		time_source_create(time_source_game, resumeDelay/1000,
		time_source_units_seconds, _tsFun, [], 1, time_source_expire_after);
	timesourceSyncVideo = 
		time_source_create(time_source_game, 0.1,
		time_source_units_seconds, function() {
			if(bgVideoLoaded) {
	        	safe_video_seek_to(clamp(nowTime, 0, bgVideoLength));
	        }
		}, [], 1);
	
#endregion

#region Layouts Vars

// Target Line

    targetLineBelow = 137*global.resolutionH/1080;
    targetLineBeside = 112*global.resolutionW/1920;
    targetLineBelowH = 6;
    targetLineBesideW = 4;
    
    function _position_update() {
        targetLineBelow = 137*global.resolutionH/1080;
        targetLineBeside = 112*global.resolutionW/1920;
    }

// Top Progress Bar

    topBarH = 5*global.scaleYAdjust;
    topBarMouseH = 20*global.scaleYAdjust;
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
    
    mixerX = array_create(2, global.resolutionH/2);
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
        for(var i=chartNotesArrayAt; i<chartNotesCount
        	&& (chartNotesArray[i].time - nowTime) * playbackSpeed / global.resolutionW <= MIXER_REACTION_RANGE; i++)
        	if(chartNotesArray[i].side == side) {
        		var _note = chartNotesArray[i];
        		if(!found) {
        			found = true;
        			beginTime = _note.time;
        		}
        		if(_note.time - beginTime > MIXER_AVERAGE_TIME_THRESHOLD)
        			break;
        		result += _note.position * (1 / _note.width);
        		accum += 1/_note.width;
        	}
        if(!found) return undefined;
        result /= accum;
        return {x: note_pos_to_x(result, side), time: beginTime};
    }

#endregion

#region Chart Vars

    chartTitle = "Last Train at 25 O'Clock"
    chartBeatPerMin = 180;      // For xml export.
    chartBarPerMin = 180/4;     // For xml export.
    chartBarOffset = 0;         // For xml export.
    chartTimeOffset = 0;        // For xml export.
    chartDifficulty = 0;
    chartSideType = ["MIXER", "MULTI"];
    chartID = "";
    chartMusicFile = "";
    chartFile = "";
    
    chartNotesArray = [];				// Type is objNote.get_prop()'s return struct.
    /// @type {Array<Array<Any>>} Activated notes' inst in a step.
    chartNotesArrayActivated = [[], [], []];
    chartNotesArrayAt = 0;
    chartNotesCount = 0;
    chartNotesMap = array_create(3);
    for(var i=0; i<3; i++)
        chartNotesMap[i] = ds_map_create();

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
    
    showDebugInfo = debug_mode;
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
        bottomDim = 0.5;
        bottomBgBlurIterations = 3;
        bottomInfoSurf = -1;
    
    // Background
        bgDim = 0.65;
        
        // Image
        bgImageFile = "";
        bgImageSpr = -1;
        
        // Video
        bgVideoLoaded = false;
        bgVideoDisplay = false;
        bgVideoPaused = false;
        bgVideoLength = 0;
        bgVideoPath = "";
        bgVideoReloading = false;
        bgVideoDestroying = false;
        bgVideoSurf = undefined;
        bgVideoAlpha = 0;
        
        safe_video_init();
        
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

		kawaseArr = kawase_create(global.resolutionW, targetLineBelow, bottomBgBlurIterations);

#endregion

#region Particles Init

    // PartSys
    partSysNote = part_system_create();
    partAlphaMul = 0.75;
    part_system_automatic_draw(partSysNote, false);

    // PartType
    
        // Note
        function _parttype_noted_init(_pt, _scl = 1.0, _ang = 0.0, _mixer = false) {
        	var _theme = theme_get();
            part_type_sprite(_pt, _theme.partSpr, false, true, false);
            if(_theme.partBlend)
            	part_type_alpha3(_pt, partAlphaMul, 0.6 * partAlphaMul, 0);
            else
            	part_type_alpha3(_pt, 1, 1, 0);
            
            if(_mixer) {
                part_type_speed(_pt, _scl * 15 * global.fpsAdjust, _scl * 15 * global.fpsAdjust, 0, 0);
                part_type_life(_pt, room_speed*0.15, room_speed*0.2);
                part_type_size(_pt, 0.7, 0.9, -0.04 * global.fpsAdjust, 0);
            }
            else {
                part_type_speed(_pt, _scl * 20 * global.fpsAdjust, _scl * 20 * global.fpsAdjust, 0, 0);
                part_type_life(_pt, room_speed*0.2, room_speed*0.4);
                part_type_size(_pt, 1.0, 1.2, -0.03 * global.fpsAdjust, -0.04 * global.fpsAdjust);
            }
            part_type_color3(_pt, c_white, _theme.partColA, _theme.partColB);
            // part_type_color2(_pt, 0x652dba, themeColor);
            // part_type_color2(_pt, _theme.partColA, _theme.partColB);
            part_type_scale(_pt, _scl * 2, _scl * 2);
            part_type_orientation(_pt, 0, 360, 2 * global.fpsAdjust, 0, false);
            part_type_blend(_pt, _theme.partBlend);
            part_type_direction(_pt, _ang, _ang, 0, 0);
        }
        
        partTypeNoteDL = part_type_create();
        
        partTypeNoteDR = part_type_create();
        
        
        // Hold
        function _parttype_hold_init(_pt, _scl = 1.0, _ang = 0.0) {
        	var _theme = theme_get();
            part_type_sprite(_pt, _theme.partSpr, false, true, false);
            if(_theme.partBlend)
            	part_type_alpha3(_pt, 0.6 * partAlphaMul, 0.6 * 0.6 * partAlphaMul, 0);
            else
            	part_type_alpha3(_pt, 1, 0.6, 0);
            part_type_speed(_pt, _scl * 15 * global.fpsAdjust
                            , _scl * 20 * global.fpsAdjust,
                            _scl * -0.825 * global.fpsAdjust * global.fpsAdjust, 0);
            part_type_color2(_pt, _theme.partColHA, _theme.partColHB);
            // part_type_color2(_pt, 0x89ffff, 0xffffe5)
            part_type_size(_pt, 0.8, 1.2, -0.02 * global.fpsAdjust, 0);
            // part_type_scale(_pt, _scl * 2, _scl * 2);
            part_type_orientation(_pt, 0, 360, 0.5 * global.fpsAdjust, 0, false);
            part_type_life(_pt, room_speed*0.2, room_speed*0.4);
            part_type_blend(_pt, _theme.partBlend);
            part_type_direction(_pt, _ang, _ang+180, 0, 0);
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
    perfRight = instance_create_depth(resor_to_x(1212/1920), resor_to_y(636/1080), 
        perfDepth, objPerfectIndc);
        
#endregion

#region Editor Init

    editor = instance_create_depth(0, 0, -10000, objEditor);

#endregion

#region TopBar Init

	topbar = instance_create_depth(0, 0, 0, objTopBar);

#endregion

// FMODGMS Related

    sampleRate = 0;
    channel = undefined;
    music = undefined;
    channelPaused = false;		// Only used for time correction
    musicLength = 0;
    usingMP3 = false;			// For Latency Workaround
    usingPitchShift = false;
    
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
			time -= pix_to_note_time(global.resolutionH - targetLineBelow - inbound);
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
	return time>nowTime && note_time_to_pix(time - nowTime) + targetLineBelow < global.resolutionH;
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
	// Resync with fmod's position because of fmod's lower precision
    nowTime = sfmod_channel_get_position(channel, sampleRate);
    
    // Sync with the video position
    if(bgVideoLoaded) {
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
	
}

#endregion

#region Draw Extra Variables

drawQueue = [array_create(1024), array_create(1024), array_create(1024)];

#endregion

_create_channel();	// Channel init.