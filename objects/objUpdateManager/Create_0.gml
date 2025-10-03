
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
if(global.autoupdate)
	_update_get_event_handle = http_get("https://api.github.com/repos/NordLandeW/DyNode/releases/latest");
