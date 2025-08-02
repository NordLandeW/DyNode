/// @description Startup Initialization

// Macros

#macro VERSION "v0.1.18.1"
#macro BASE_RES_W 1920
#macro BASE_RES_H 1080
#macro BASE_FPS 60
#macro MAXIMUM_DELAY_OF_SOUND 20        	// in ms
#macro MAXIMUM_UNDO_STEPS 3000
#macro EPS 0.01
#macro MIXER_REACTION_RANGE 0.35			// Mixer's reaction pixel range's ratio of resolutionW
#macro SYSFIX "\\\\?\\"						// Old system prefix workaround for win's file path
#macro VIDEO_FREQUENCY (global.VIDEO_UPDATE_FREQUENCY)			// in hz
#macro EXPORT_XML_EPS 6
#macro LERP_EPS 0.001
#macro INF 0x7fffffff
#macro USE_DSP_PITCHSHIFT (objMain.usingPitchShift)
#macro MAX_SELECTION_LIMIT 400
#macro KPS_MEASURE_WINDOW 400
#macro AUTOSAVE_TIME (global.autoSaveTime)	// in seconds
#macro DYCORE_BUFFER_SIZE (50*1024*1024)	// 50MB
#macro DYCORE_COMPRESSION_LEVEL (global.PROJECT_COMPRESSION_LEVEL)		// max = 22
#macro DYCORE_BUFFER_ADDRESS (buffer_get_address(global.__DyCore_Buffer))
#macro DEBUG_MODE (true)
math_set_epsilon(0.00000001);				// 1E-8

// Announcement init
/// @type {Array<Id.Instance.objAnnouncement>} 
announcements = [];

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
global.particleEffects = true;

// Advanced settings
global.FMOD_MP3_DELAY = 0;
global.ANNOUNCEMENT_MAX_LIMIT = 7;
global.VIDEO_UPDATE_FREQUENCY = 60;
global.PROJECT_COMPRESSION_LEVEL = 11;		// zstd compression level. (0~22)

// Singletons init
global.activationMan = new Activationmanager();

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
        show_error_async(i18n_get("FMOD_create_sys_err") + FMODGMS_Util_GetErrorMessage(), false);
    }
    
    // Initialize the system
    FMODGMS_Sys_Set_DSPBufferSize(1024, 4);
    FMODGMS_Sys_Initialize(32);
    
    // Create the pitch shift effect
    global.__DSP_Effect = FMODGMS_Effect_Create(FMOD_DSP_TYPE.FMOD_DSP_TYPE_PITCHSHIFT);
    if(global.__DSP_Effect < 0)
    	announcement_error($"FMOD Cannot create pitchshift effect.\nMessage:{FMODGMS_Util_GetErrorMessage()}");
    else {
    	// FMODGMS_Effect_Set_Parameter(global.__DSP_Effect, FMOD_DSP_PITCHSHIFT.FMOD_DSP_PITCHSHIFT_FFTSIZE, 4096);
    	// FMODGMS_Effect_Set_Parameter(global.__DSP_Effect, FMOD_DSP_PITCHSHIFT.FMOD_DSP_PITCHSHIFT_MAXCHANNELS, 10);
    }
    // FMODGMS_Sys_Set_SoftwareFormat(48000, 0);
    
// DyCore Initialization

if(DyCore_init() != "success") {
	show_error("DyCore Initialized Failed.", false);
}

global.__DyCore_Buffer = buffer_create(DYCORE_BUFFER_SIZE, buffer_fixed, 1);
global.__DyCore_Manager = new DyCoreManager();

// Input Initialization

global.__InputManager = new InputManager();

// Fonts Initialization

global._notoFont = font_add("fonts/notosanscjkkr-black.otf", 30, false, false, 32, 65535);
if(!font_exists(global._notoFont))
	show_error("Font load failed.", true);
font_enable_sdf(global._notoFont, true);
scribble_anim_cycle(0.2, 255, 255);
scribble_font_bake_shadow("fOrbitron48", "fOrbitron48s", 0, 10, c_black, 0.4, 0, true);
scribble_font_bake_shadow("fDynamix16", "fDynamix16s", 0, 2, c_black, 0.3, 0, true);
scribble_font_bake_outline_8dir("fDynamix16", "fDynamix16o", c_white, true);
	// Prefetch Msdf Fonts' Texture (this takes time)
	var _texarr = texturegroup_get_textures("texFonts");
	for(var i=0, l=array_length(_texarr); i<l; i++) {
		texture_prefetch(_texarr[i]);
	}

// Window Init

windowDisplayRatio = 0.7;
window_enable_borderless_fullscreen(true);

window_set_fullscreen(global.fullscreen);
if(os_type == os_windows)
	window_command_hook(window_command_close);

// Randomize

randomize();

// Debug Layer Init

// if(debug_mode)
// 	show_debug_overlay(true);

// Init finished

GoogHit("login", {
	version: VERSION, 
	session_id: random_id(16),
	engagement_time_msec: "100"}); // Analytics: Version

if(DEBUG_MODE) test_at_start();

if(DEBUG_MODE) room_goto(rMain);
else
	room_goto(rStartPage);

#region Project Properties

	// chartPath is deprecated in version v0.1.16
	projectPath = "";
	backgroundPath = "";
	musicPath = "";
	videoPath = "";
	projectTime = 0;		// in ms
	
#endregion

#region Inner Variables

	annoThresholdNumber = 7;
	annoThresholdTime = 400;
	annosY = [];
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

#region Updater Init / Variables

#macro SOURCE_IORI "https://d.g.iorinn.moe/dyn/"
#macro UPDATE_TARGET_FILE (program_directory + "update.zip")
#macro UPDATE_TEMP_DIR (program_directory + "tmp/")

// Event handles
_update_get_event_handle = undefined;
_update_download_event_handle = undefined;
_update_unzip_event_handle = undefined;

_update_version = "";
_update_url = "";
_update_filename = "";
_update_github_url = "";

enum UPDATE_STATUS {
	IDLE,
	CHECKING_I,
	CHECKING_II,
	DOWNLOADING,
	UNZIP,
	READY,
	FAILED
};

/// @type {Enum.UPDATE_STATUS} 
_update_status = UPDATE_STATUS.IDLE;

// For download progress bar
_update_received = 0;
_update_size = 0;

// Update functions
function update_cleanup() {
	var _status = DyCore_cleanup_tmpfiles(program_directory);
	if(_status < 0)
		show_debug_message("Cleanup error.");
}

function start_update() {
	if(_update_status != UPDATE_STATUS.IDLE)
		return;
	_update_status = UPDATE_STATUS.CHECKING_I;
	_update_download_event_handle = http_get_file(_update_github_url, UPDATE_TARGET_FILE);
	announcement_play("autoupdate_process_2");
}

function fallback_update() {
	_update_status = UPDATE_STATUS.CHECKING_II;
	_update_download_event_handle = http_get_file(SOURCE_IORI + _update_filename, UPDATE_TARGET_FILE);
	announcement_play("autoupdate_process_3");
}

function start_update_unzip() {
	_update_status = UPDATE_STATUS.UNZIP;
	_update_unzip_event_handle = zip_unzip_async(UPDATE_TARGET_FILE, UPDATE_TEMP_DIR);
}

function update_ready() {
	_update_status = UPDATE_STATUS.READY;

	announcement_play("autoupdate_process_4");
}

function skip_update() {
	global.lastCheckedVersion = _update_version;

	announcement_play("autoupdate_skip");
}

function stop_autoupdate() {
	if(global.autoupdate) {
		global.autoupdate = false;
		announcement_play("autoupdate_remove");
	}
}

// Check For Update
update_cleanup();
if(global.autoupdate && !DEBUG_MODE)
	_update_get_event_handle = http_get("https://api.github.com/repos/NordLandeW/DyNode/releases/latest");

#endregion

