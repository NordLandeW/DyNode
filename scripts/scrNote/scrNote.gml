
function NoteIDManager() constructor {

	/// @type {Any} Map noteID to instance.ID.
	noteIDMap = {};

	static update = function(noteID, instID) {
		noteIDMap[$ noteID] = instID;
	}

	/// @description Get the instance ID by noteID.
	/// @param {String} noteID
	/// @returns {Id.Instance.objNote} The instance ID of the note.
	static get = function(noteID) {
		if(!variable_struct_exists(noteIDMap, noteID)) {
			return -1;
		}
		return noteIDMap[$ noteID];
	}

	static remove = function(noteID) {
		if(variable_struct_exists(noteIDMap, noteID)) {
			variable_struct_remove(noteIDMap, noteID);
		}
	}

	static clear = function() {
		delete noteIDMap;
		noteIDMap = {};
	}
}

global.noteIDMan = new NoteIDManager();

#macro NOTE_ID_LENGTH 9

function _outbound_check(_x, _y, _side) {
    if(_side == 0 && _y < -100)
        return true;
    else if(_side == 1 && _x >= BASE_RES_W / 2)
        return true;
    else if(_side == 2 && _x <= BASE_RES_W / 2)
        return true;
    else
        return false;
}

function _outroom_check(_x, _y) {
	return !pos_inbound(_x, _y, 0, 0, BASE_RES_W, BASE_RES_H);
}

function _outbound_check_t(_time, _side) {
    var _pos = note_time_to_y(_time, _side);
    if(_side == 0 && _pos < -100)
        return true;
    else if(_side == 1 && _pos >= BASE_RES_W / 2)
        return true;
    else if(_side == 2 && _pos <= BASE_RES_W / 2)
        return true;
    else
        return false;
}

function _outscreen_check(_x, _y, _side) {
	return _side == 0? !in_between(_x, 0, BASE_RES_W) : !in_between(_y, 0, BASE_RES_H);
}

function note_recac_stats() {
	if(!(in_between(objMain.showStats, 1, 2)))
		return;

	var result = json_parse(DyCore_note_count());
	objMain.statCount = result;
}

function note_sort_all(forced = false) {
	if(!forced && !objEditor.editorNoteSortRequest) return;
	objEditor.editorNoteSortRequest = false;
	
	var result = DyCore_sort_notes();
	
	if(result == 0) {
		note_recac_stats();
	}
	
	objMain.chartNotesCount = DyCore_get_note_count();
}

function note_sort_request() {
	objEditor.editorNoteSortRequest = true;
}

function build_note(prop, record = false, selecting = false, randomID = false) {
	prop = SnapDeepCopy(prop);
	var _isHold = prop.noteType == NOTE_TYPE.HOLD;
	if(!variable_struct_exists(prop, "noteID") || prop.noteID == "")
		randomID = true;

	if(randomID) {
		prop.noteID = note_generate_id();
		if(_isHold) {
			prop.subNoteID = note_generate_id();
		}
	}
	if(!_isHold) prop.subNoteID = "";

	var _newNote = new sNoteProp(prop);

	switch(prop.noteType) {
		case NOTE_TYPE.NORMAL:
		case NOTE_TYPE.CHAIN:
			dyc_create_note(_newNote, record);
			break;
		case NOTE_TYPE.HOLD:
			var _newSubNote = new sNoteProp({
				time: prop.time + prop.lastTime,
				side: prop.side,
				width: prop.width,
				position: prop.position,
				lastTime: prop.lastTime,
				noteType: NOTE_TYPE.SUB,
				noteID: prop.subNoteID,
				subNoteID: prop.noteID,
				beginTime: prop.time
			});
			dyc_create_note(_newSubNote, false);
			dyc_create_note(_newNote, record);
			break;
		case NOTE_TYPE.SUB:
			throw "Cannot directly create sub notes."
	}
}

// This function can only be accessed in destroy event.
/// @param {String} noteID
function note_delete(noteID, _record = false) {
	if(noteID == "") {
		return;
	}

	if(note_is_activated(noteID)) {
		var _inst = note_get_instance(noteID);
		note_deactivate_instance(_inst);
	}

	try {
		var _prop = dyc_get_note(noteID);
		if(_record)
			operation_step_add(OPERATION_TYPE.REMOVE, _prop, -1);
		var result = DyCore_delete_note(noteID);
		if(_prop.noteType == NOTE_TYPE.HOLD) {
			note_delete(_prop.subNoteID, false);
		}
		if(result < 0)
			throw "DyCore note deletion failed.";
	} catch (e) {
		announcement_error($"音符删除出现错误。请将谱面导出并备份以避免问题进一步恶化。您可以选择将该信息反馈开发者以帮助我们解决问题。错误信息：{e}");
	}
    
    note_sort_request();
}

function note_delete_all() {
	with(objMain) {
		chartNotesArrayAt = 0;
		chartNotesCount = 0;
		
		instance_destroy(objNote);
		DyCore_clear_notes();
	}
}

function note_delete_all_manually(_record = true) {
	for(var i=0, l= objMain.chartNotesCount; i<l; i++) {
		var _str = dyc_get_note_at_index(i);
		note_delete(_str.noteID, _record);
	}
}

function note_check_and_activate(_posistion_in_array) {
	var _str = dyc_get_note_at_index(_posistion_in_array);
	if(note_is_activated(_str.noteID)) {
		return 0;
	}
	var _note_inbound = !_outbound_check_t(_str.time, _str.side) && _str.time >= objMain.nowTime;
	var _hold_intersect = _str.noteType >= 2 &&
		(_str.noteType == 2? (_str.time <= objMain.nowTime && _str.time + _str.lastTime >= objMain.nowTime):
			(_str.beginTime <= objMain.nowTime && _str.time >= objMain.nowTime));
	if(_note_inbound || _hold_intersect) {
		note_activate(_str.noteID);
		return 1;
	}
	else if(!_note_inbound && _outbound_check_t(_str.time, !(_str.side))) {
		return -1;
	}
	return 0;
}

/// @param {Id.Instance.objNote} inst
function note_deactivate_instance(inst) {
	if(!note_is_activated(inst)) return;
	inst.detach();

	// ! Only for now. 
	// Release the instance back to objectPool.
	if(inst.noteType == NOTE_TYPE.HOLD)
		instance_destroy(inst.sinst);
	instance_destroy(inst);
}

/// @param {Id.Instance.objNote} inst 
function note_activate_instance(inst) {
	if(note_is_activated(inst)) return;
	instance_activate_object(inst);
	inst.pull_prop();
	global.activationMan.activate(inst);
}

function note_activate(noteID, trackHead = true) {
	if(note_is_activated(noteID)) return;
	// Get note object from objectPool.
	var _prop = dyc_get_note(noteID);
	if(_prop.noteType == NOTE_TYPE.SUB && trackHead) {
		// If is a sub note, get the main note.
		var _mainNote = dyc_get_note(_prop.subNoteID);
		if(_mainNote == undefined) {
			throw "Sub note " + noteID + " has no main note.";
			return;
		}
		if(note_is_activated(_mainNote.noteID)) {
			// If the main note is already activated, just return.
			return;
		}
		_prop = _mainNote;
	}

	/// @type {Id.Instance.objNote} 
	var _inst = instance_create_depth(0, 0, 0, _note_get_object_asset(_prop.noteType));
	_inst.attach(noteID);
	global.activationMan.track(_inst);

	// show_debug_message("Trying activate the note: " + noteID);
}

function note_deactivate_all() {
	global.activationMan.deactivate_all();
	instance_deactivate_object(objNote);
}

function note_get_instance(noteID) {
	return global.noteIDMan.get(noteID);
}

function note_exists(inst_or_noteID) {
	// If is noteID
	if(is_string(inst_or_noteID)) {
		inst_or_noteID = note_get_instance(inst_or_noteID)
		if(inst_or_noteID < 0) return false;
	}
	return global.activationMan.is_tracked(inst_or_noteID);
}

function note_is_activated(inst_or_noteID) {
	// If is noteID
	if(is_string(inst_or_noteID)) {
		inst_or_noteID = note_get_instance(inst_or_noteID)
		if(inst_or_noteID < 0) return false;
	}
	return global.activationMan.is_activated(inst_or_noteID);
}

function note_select_reset(inst = undefined) {
	if(inst == undefined) inst = objNote;
	/// @self Id.Instance.objNote
	with(inst)
		if(stateType == NOTE_STATES.SELECTED) {
			set_state(NOTE_STATES.NORMAL);
			state();
		}
}

function note_activation_reset() {
	note_deactivate_all();
	with(objMain) {
		for(var i=0; i<chartNotesCount; i++)
			note_check_and_activate(i);
	}
}

function note_generate_id() {
	return DyCore_random_string(NOTE_ID_LENGTH);
}