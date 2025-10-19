
var _nx = x + shiftInX * animcurve_channel_evaluate(shiftCurv, min(anniTimer/shiftTime, 1));
var _ny = y - nowShiftY;
var _col = theme_get().color;
var _borderthick = sin(anniTimer * 2 * pi / ANNO_BORDER_ANIMATION_TIME) * 2 + 2;
var _borderalp = sin(anniTimer * 2 * pi / ANNO_BORDER_ANIMATION_TIME) * 0.5 + 0.5;
var _bordercol = merge_color(_col, c_white, 0.5);

var _bbox = element.get_bbox(_nx, _ny);

if(mouse_inbound(_bbox.left, _bbox.top, _bbox.right, _bbox.bottom))
	image_alpha = 1;

switch(annoState) {
	case ANNO_STATE.ERROR:
		_col = c_red;
		break;
	case ANNO_STATE.COMPLETE:
		_col = c_green;
		break;
	default:
		break;
}

switch (annoType) {
	case ANNO_TYPE.NORMAL:
		draw_scribble_anno_box(element, _nx, _ny, _col, image_alpha * 0.6);
		break;
	case ANNO_TYPE.TASK:
		switch(annoState) {
			default:
				draw_scribble_anno_box(element, _nx, _ny, _col, image_alpha * 0.6);
				break;
			case ANNO_STATE.PROCESSING:
				draw_scribble_anno_box_ext(element, _nx, _ny, _col, image_alpha * 0.6, 
										   _borderthick, _bordercol, _borderalp * image_alpha);
				break;
		}
		break;
}

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