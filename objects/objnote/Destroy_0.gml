/// @description Delete id in array and map

if(noteType == 2 && note_exists(sinst)) {
	note_activate(sinst);
	instance_destroy(sinst);
}

note_delete(noteID, recordRequest);
global.activationMan.deactivate(id);

if(stateType == NOTE_STATES.ATTACH_SUB || stateType == NOTE_STATES.DROP_SUB) {
	editor_lrside_lock_set(false);
}