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
var ww = window_get_width(), wh = window_get_height();
var aw = surface_get_width(application_surface), ah = surface_get_height(application_surface);
var scalar = min(ww / BASE_RES_W, wh / BASE_RES_H);
var nw = BASE_RES_W * scalar, nh = BASE_RES_H * scalar;

if(nw > 0 && nh > 0 && aw != nw && ah != nh) {
	surface_resize(application_surface, nw, nh);

	view_wport[0] = nw;
	view_hport[0] = nh;
	view_xport[0] = 0;
	view_yport[0] = 0;

}