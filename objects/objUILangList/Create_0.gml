/// @description 

// Inherit the parent event
event_inherited();

lists = variable_instance_get(id, "lists");

for(var i=0; i<listCount; i++)
	if(lists[i] == i18n_get_lang()) {
		listPointer = i;
		break;
	}
input = i18n_get_title();

listFunction = function (_input) {
	i18n_set_lang(_input);
	input = i18n_get_title();
	room_restart();
}