/// @description Delete id in array and map

if(noteType == 2 && note_exists(sinst))
	instance_destroy(sinst);

global.activationMan.deactivate(id);

if(stateType == NOTE_STATES.ATTACH_SUB || stateType == NOTE_STATES.DROP_SUB) {
	editor_lrside_lock_set(false);
}