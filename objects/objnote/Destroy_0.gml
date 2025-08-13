/// @description Delete id in array and map

// If is dummy note, remove sinst manually.
if(noteID == "") {
	if(note_exists(sinst))
		instance_destroy(sinst);
}

global.activationMan.deactivate(id);

if(stateType == NOTE_STATES.ATTACH_SUB || stateType == NOTE_STATES.DROP_SUB) {
	editor_lrside_lock_set(false);
}