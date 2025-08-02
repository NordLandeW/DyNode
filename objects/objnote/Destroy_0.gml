/// @description Delete id in array and map

global.activationMan.deactivate(id);

if(stateType == NOTE_STATES.ATTACH_SUB || stateType == NOTE_STATES.DROP_SUB) {
	editor_lrside_lock_set(false);
}