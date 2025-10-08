global.frameCurrentTime = get_timer();

// Input Update
if(global.__InputManager != undefined)
	global.__InputManager.step();

// GUI Update
if(global.__GUIManager != undefined) {
	global.__GUIManager.step();
}

// DyCore Update
if(global.__DyCore_Manager != undefined) {
	global.__DyCore_Manager.step();
}

// Window resolution Update
if(!global.recordManager.is_recording()) {
	var ww = window_get_width(), wh = window_get_height();
	var aw = surface_get_width(application_surface), ah = surface_get_height(application_surface);
	var scalar = min(ww / BASE_RES_W, wh / BASE_RES_H);
	var nw = BASE_RES_W * scalar, nh = BASE_RES_H * scalar;
	
	if(nw > 0 && nh > 0 && aw != nw && ah != nh) {
		resolution_set(nw, nh);
	}
}