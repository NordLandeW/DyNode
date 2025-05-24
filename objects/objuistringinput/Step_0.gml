/// @description 

// Inherit the parent event
event_inherited();

description = variable_instance_get(id, "description");

var _inbound = mouse_inbound(x, y, x+maxWidth, y+scriHeight*2);

if(_inbound && mouse_isclick_l()) {
	input = get_string_i18n(i18n_get(description), "");
}