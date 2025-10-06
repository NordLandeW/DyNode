/// @description Input check & Exts update

#region Window command related

if(os_type == os_windows) {
	if(keycheck_down(vk_f7)) {
		global.fullscreen = !global.fullscreen;
		window_set_fullscreen(global.fullscreen);
	}
	
	if(window_command_check(window_command_close)) {
		game_end_confirm();
	}
}
else {
	if(keycheck_down(vk_f7)) {
		if(keycheck_down(vk_f7))
			global.fullscreen = !global.fullscreen;
		window_set_fullscreen(global.fullscreen);
	}
}

#endregion

#region Announcement update

// Clear removed annos
for(var i=0, l=array_length(announcements); i<l; i++) {
	if(!instance_exists(announcements[i])) {
		array_delete(announcements, i, 1);
		i--;
		l--;
	}
}

// Caculate annos' Y
var _h = 0;
var _margin = 15;
var _l = array_length(announcements);
for(var i=array_length(announcements)-1; i>=0; i--) {
	/// @self Id.Instance.objAnnouncement
	with(announcements[i]) {
		targetY = _h;
		_h += element.get_bbox().height + _margin;
		
		if(_l - i > objManager.annoThresholdNumber)
			lastTime = min(lastTime, timer + objManager.annoThresholdTime);
	}
}


#endregion


camera_set_view_size(view_camera[0], BASE_RES_W, BASE_RES_H);

var _fmoderr = FMODGMS_Sys_Update();

if(_fmoderr < 0) {
    show_debug_message("FMOD ERROR:\n"+FMODGMS_Util_GetErrorMessage());
}

if(keycheck_down_ctrl(vk_f11)) {
	debugLayer = !debugLayer;
	show_debug_overlay(debugLayer);
}
	
	

if(room == rMain) {
	if(keycheck_down(vk_f2))
    	map_load();
    if(keycheck_down_ctrl(ord("S"))) {
		if(!shift_ishold())
    		project_save();
		else if(shift_ishold() && !alt_ishold())
			project_save_as();
	}
	if(keycheck_down(vk_f1))
	    project_load();
	if(keycheck_down_ctrl(ord("N")))
		project_new();
	
	
	
	// For New Project Initialization --- related codes in rStartPage and rProjectInit
		// If there is a init struct
		if(initVars != undefined) {
			var _str = initVars;
			dyc_chart_set_metadata({
				title: _str.title,
				sideType: [_str.ltype, _str.rtype],
				difficulty: difficulty_char_to_num(string_char_at(_str.diff, 1)),
				charter: "",
				artist: ""
			});
			if(_str.mus != "")
				music_load(_str.mus);
			if(_str.bg != "") background_load(_str.bg);
			if(_str.chart != "") map_load(_str.chart);
			initVars = undefined;
		}
		
		// Or there is a init project
		if(initWithProject) {
			initWithProject = false;
			
			if(!project_load()) room_goto(rStartPage);
		}
}    
    
if(keycheck_down_ctrl(vk_f12)) {
	var _file = SYSFIX + program_directory + "Screenshots\\" + random_id(9) + ".png"
	screen_save(_file);
	announcement_play(i18n_get("screenshot_save") + _file)
}

else if(keycheck_down(vk_f12)) {
	url_open("https://dyn.iorinn.moe/shortcuts.html");
}

if(keycheck_down(vk_f8))
	switch_autosave();

if(keycheck_down(vk_f9))
	theme_next();

if(keycheck_down(vk_escape)) {
	if(!instance_exists(objEditor))
		game_end_confirm();
}

// Debug functions

if(DEBUG_MODE) {
	if(keycheck_down_ctrl(vk_numpad0))
		project_auto_save();
}

// Update project time

if(delta_time < 1000000)
	projectTime += delta_time / 1000;