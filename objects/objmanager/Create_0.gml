/// @description Startup Initialization

// Macros

#macro VERSION DyCore_get_version()
#macro BASE_RES_W 1920
#macro BASE_RES_H 1080
#macro BASE_FPS 60
#macro MAXIMUM_DELAY_OF_SOUND 20        	// in ms
#macro MAXIMUM_UNDO_STEPS 3000
#macro EPS 0.01
#macro MIXER_REACTION_RANGE 0.35			// Mixer's reaction pixel range's ratio of resolutionW
#macro SYSFIX "\\\\?\\"						// Old system prefix workaround for win's file path
#macro EXPORT_XML_EPS 6
#macro LERP_EPS 0.001
#macro INF 0x7fffffff
#macro MAX_SELECTION_LIMIT 1500
#macro KPS_MEASURE_WINDOW 2000
#macro AUTOSAVE_TIME (global.autoSaveTime)	// in seconds
#macro DYCORE_COMPRESSION_LEVEL (global.PROJECT_COMPRESSION_LEVEL)		// max = 22
#macro DEBUG_MODE (debug_mode)
#macro FMOD_DSP_BUFFERSIZE (256)
#macro FMOD_DSP_BUFFERCOUNT (4)
#macro FMOD_DSP_APP_PITCHSHIFT_FFTSIZE (4096)
math_set_epsilon(0.00000001);				// 1E-8

// Global Configs
global.fps = display_get_frequency();
global.autosave = false;
global.autoupdate = true;
global.fullscreen = false;
global.simplify = false;
global.updatechannel = "STABLE";		// STABLE / BETA (not working for now)
global.beatlineStyle = BeatlineStyles.BS_DEFAULT;
global.musicDelay = 0;
global.graphics = {
	AA : 2,
	VSync : true
};
global.dropAdjustError = 0.125;
global.offsetCorrection = 2;
global.lastCheckedVersion = "";
global.autoSaveTime = 60 * 3;
global.analytics = true;
global.particleEffects = 1;	// 0: off, 1: full, 2: low

// Advanced settings
global.FMOD_MP3_DELAY = 0;
global.ANNOUNCEMENT_MAX_LIMIT = 7;
global.PROJECT_COMPRESSION_LEVEL = 11;		// zstd compression level. (0~22)

// Singletons init
global.activationMan = new Activationmanager();
global.recordManager = new RecordManager();
global.announcementMan = new AnnouncementManager();

// Themes Init

theme_init();

// Localization Init

i18n_init();

// Load Settings

if(DEBUG_MODE) save_config();

_lastConfig_md5 = load_config();

// Global Variables

global.difficultyName = ["CASUAL", "NORMAL", "HARD", "MEGA", "GIGA", "TERA"];
global.difficultySprite = [sprCasual, sprNormal, sprHard, sprMega, sprGiga, sprTera];
global.difficultyString = "CNHMGT";
global.difficultyCount = string_length(global.difficultyString);

global.noteTypeName = ["NORMAL", "CHAIN", "HOLD", "SUB"];
global.__GUIManager = undefined;

global.shadowCount = 0;
// Used to distinguish different frames. Updated on begin step event.
global.frameCurrentTime = 0;
// Used to prevent multiple saves from happening at once.
global.isSaving = false;

// Generate Temp Sprite

global.sprLazer = generate_lazer_sprite(2048);
global.sprHoldBG = generate_hold_sprite(BASE_RES_W + 4*sprite_get_height(sprHold));
global.sprPauseShadow = generate_pause_shadow(200);

// Set GUI & Window Resolution

surface_resize(application_surface, BASE_RES_W, BASE_RES_H);
display_set_gui_size(BASE_RES_W, BASE_RES_H);

// Graphics settings init
gpu_set_tex_filter(true);
display_reset(global.graphics.AA, global.graphics.VSync);

// FMODGMS Initialization

    // Optional: Check to see if FMODGMS has loaded properly
    if (FMODGMS_Util_Handshake() != "FMODGMS is working.") {
        announcement_error("FMOD_load_err");
    }
    
    // Create the system
    if (FMODGMS_Sys_Create() < 0) {
        show_error(i18n_get("FMOD_create_sys_err") + FMODGMS_Util_GetErrorMessage(), false);
    }
    
    // Initialize the system
    FMODGMS_Sys_Set_DSPBufferSize(FMOD_DSP_BUFFERSIZE, FMOD_DSP_BUFFERCOUNT);
    FMODGMS_Sys_Initialize(32);
    
    // Create the pitch shift effect
    global.__DSP_Effect = FMODGMS_Effect_Create(FMOD_DSP_TYPE.FMOD_DSP_TYPE_PITCHSHIFT);
    if(global.__DSP_Effect < 0)
    	announcement_error($"FMOD Cannot create pitchshift effect.\nMessage:{FMODGMS_Util_GetErrorMessage()}");
    else {
    	FMODGMS_Effect_Set_Parameter(global.__DSP_Effect, FMOD_DSP_PITCHSHIFT.FMOD_DSP_PITCHSHIFT_FFTSIZE, FMOD_DSP_APP_PITCHSHIFT_FFTSIZE);
    	FMODGMS_Effect_Set_Parameter(global.__DSP_Effect, FMOD_DSP_PITCHSHIFT.FMOD_DSP_PITCHSHIFT_MAXCHANNELS, 2);
    }
    
// DyCore Initialization

dyc_init();

global.__DyCore_Manager = new DyCoreManager();

// Input Initialization

global.__InputManager = new InputManager();

// Fonts Initialization

global._notoFont = font_add("fonts/notosanscjkkr-black.otf", 30, false, false, 32, 65535);
if(!font_exists(global._notoFont))
	show_error("Font load failed.", true);
font_enable_sdf(global._notoFont, true);
scribble_anim_cycle(0.2, 255, 255);

// Window Init

windowDisplayRatio = 0.8;
window_reset();
window_enable_borderless_fullscreen(true);

window_set_fullscreen(global.fullscreen);
if(os_type == os_windows)
	window_command_hook(window_command_close);

dyc_disable_ime();

// Randomize

randomize();

// Debug Layer Init

// if(debug_mode)
// 	show_debug_overlay(true);

// Check FFmpeg Availability

if(DyCore_ffmpeg_is_available()) {
	show_debug_message("FFmpeg is available.");
}

// Console init
console_init();

// Init finished

GoogHit("login", {
	version: VERSION, 
	session_id: random_id(16),
	engagement_time_msec: "100"}); // Analytics: Version

if(DEBUG_MODE) test_at_start();

if(DEBUG_MODE) {
	room_goto(rMain);
}
else
	room_goto(rStartPage);

call_later(1, time_source_units_frames, function() {
	parameter_parse();
});

#region Project Properties

	// chartPath is deprecated in version v0.1.16
	projectPath = "";
	// temporary variables before project is saved completely
	nextProjectPath = "";
	backgroundPath = "";
	musicPath = "";
	videoPath = "";
	projectTime = 0;		// in ms
	
#endregion

#region Inner Variables

	animAnnoSpeed = 1 / room_speed;
	
	/// @type {Any} 
	initVars = undefined;
	initWithProject = false;
	
	autosaving = false;
	tsAutosave = time_source_create(time_source_game, AUTOSAVE_TIME, time_source_units_seconds, project_auto_save, [], -1);
	if(global.autosave) {
		global.autosave = false;
		switch_autosave();
	}
	
	// For config.json update
	
	tsConfigLiveChange = time_source_create(time_source_game, 1, time_source_units_seconds, 
		function() {
			with(objManager)
				if(_lastConfig_md5 != md5_config()) {
					_lastConfig_md5 = load_config();
					show_debug_message_safe("MD5: "+_lastConfig_md5);
					announcement_play("检测到配置被更改，改变后的一部分配置已经生效。");
				}
		}
		, [], -1);
	// time_source_start(tsConfigLiveChange);
	
	debugLayer = false;

#endregion

// Create Update Manager
instance_create_depth(0, 0, depth, objUpdateManager);