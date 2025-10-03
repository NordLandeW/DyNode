
var _nx = x + shiftInX * animcurve_channel_evaluate(shiftCurv, min(timer/shiftTime, 1));
var _ny = y - nowShiftY;

var _bbox = element.get_bbox(_nx, _ny);

if(mouse_inbound(_bbox.left, _bbox.top, _bbox.right, _bbox.bottom))
	image_alpha = 1;
draw_scribble_box(element, _nx, _ny, image_alpha * 0.6);

element
    .blend(c_white, image_alpha)
    .msdf_shadow(c_black, image_alpha * 0.5, 0, 2)
    .draw(_nx, _ny);

if(element.region_detect(_nx, _ny, mouse_x, mouse_y) == "update" && mouse_isclick_l()) {
	url_open(objUpdateManager._update_url);
}

if(element.region_detect(_nx, _ny, mouse_x, mouse_y) == "update_2" && mouse_isclick_l()) {
	objUpdateManager.start_update();
}

if(element.region_detect(_nx, _ny, mouse_x, mouse_y) == "update_skip" && mouse_isclick_l()) {
	objUpdateManager.skip_update();
}

if(element.region_detect(_nx, _ny, mouse_x, mouse_y) == "update_off" && mouse_isclick_l()) {
	objUpdateManager.stop_autoupdate();
}