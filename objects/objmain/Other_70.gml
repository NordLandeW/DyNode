/// @description Video Events

var _type = async_load[? "type"];

if(_type == "video_start") {
	if(!bgVideoDisplay) {
		bgVideoLoaded = true;
		bgVideoLength = video_get_duration();
		safe_video_pause();
		
		if(!bgVideoReloading)
			announcement_play("video_load_finished");
	}
	bgVideoDisplay = true;
	bgVideoReloading = false;
}