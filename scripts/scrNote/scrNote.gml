
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
    else if(_side == 1 && _x >= global.resolutionW / 2)
        return true;
    else if(_side == 2 && _x <= global.resolutionW / 2)
        return true;
    else
        return false;
}

function _outroom_check(_x, _y) {
	return !pos_inbound(_x, _y, 0, 0, global.resolutionW, global.resolutionH);
}

function _outbound_check_t(_time, _side) {
    var _pos = note_time_to_y(_time, _side);
    if(_side == 0 && _pos < -100)
        return true;
    else if(_side == 1 && _pos >= global.resolutionW / 2)
        return true;
    else if(_side == 2 && _pos <= global.resolutionW / 2)
        return true;
    else
        return false;
}

function _outscreen_check(_x, _y, _side) {
	return _side == 0? !in_between(_x, 0, global.resolutionW) : !in_between(_y, 0, global.resolutionH);
}

function note_recac_stats() {
	if(!(in_between(objMain.showStats, 1, 2)))
		return;

	stat_reset();
	var _arr = objMain.chartNotesArray
	for(var i=0, l=array_length(_arr); i<l; i++)
		if(_arr[i].time != INF) {
			stat_count(_arr[i].side, _arr[i].noteType);
		}
}

function note_sort_all() {
	var startTime = get_timer();
    var _f = function (_x) {
		return _x.time;
	}
    objMain.chartNotesArray = extern_index_sort(objMain.chartNotesArray, _f);

	var endTime = get_timer();
	show_debug_message("Note sorting took " + string((endTime - startTime)/1000) + "ms");
    
    // Update arrayPos & Flush deleted notes
	startTime = get_timer();
    with(objMain) {
    	chartNotesCount = array_length(chartNotesArray);
    	
    	for(var i=0; i<chartNotesCount; i++) {
			chartNotesArray[i].index = i;
		}
    	
		var notes_to_delete = 0, p = array_length(chartNotesArray) - 1;
    	while(p >= 0) {
			if(chartNotesArray[p].time == INF) {
				notes_to_delete++;
			}
			p--;
    	}
		
		chartNotesCount -= notes_to_delete;
		array_resize(chartNotesArray, chartNotesCount);
    }

	note_recac_stats();
	endTime = get_timer();
	show_debug_message("Note recalculation took " + string((endTime - startTime)/1000) + "ms");
}

function note_sort_request() {
	objEditor.editorNoteSortRequest = true;
}

function build_note(_type, _time, _position, _width, 
	_sinst, _side, _record = false, _selecting = false, _id = "") {
	if(_id == "") _id = note_generate_id();
	if(_sinst < 0) _sinst = -999;

    var _obj = undefined;
    switch(_type) {
        case "NORMAL":
        case 0:
            _obj = objNote;
            break;
        case "CHAIN":
        case 1:
            _obj = objChain;
            break;
        case "HOLD":
        case 2:
            _obj = objHold;
            break;
        case "SUB":
        case 3:
            _obj = objHoldSub;
            break;
        default:
            return;
    }
	/// @type {Id.Instance.objNote} 
    var _inst = instance_create_depth(0, 0, 0, _obj);
	_inst.noteID = _id;
    _inst.width = real(_width);
    _inst.side = real(_side);
	_inst.time = _time;
    _inst.position = real(_position);
	_inst.sinst = _sinst;

	if(_sinst > 0) {
		_inst.subNoteID = _sinst.noteID;
	}

	global.noteIDMan.update(_id, _inst);
    
    with(_inst) {
    	_prop_init(true);
    	if(noteType == 2) _prop_hold_update();
    	if(_selecting) set_state(NOTE_STATES.SELECTED);
    }
    with(objMain) {
        array_push(chartNotesArray, _inst.get_prop(true));
		DyCore_insert_note(json_stringify(_inst.get_prop()));        
        note_sort_request();
    }
    
    if(_record)
    	operation_step_add(OPERATION_TYPE.ADD, _inst.get_prop(), -1);
    
    return _inst;
}

function build_hold(_time, _position, _width, _subtime,
					_side, _record = false, _selecting = false, _id = "", _subid = "") {
	var _sinst = build_note(3, _subtime, _position, _width, -1, _side,
							false, _selecting, _subid);
	var _inst = build_note(2, _time, _position, _width, _sinst, 
						   _side, false, _selecting, _id);
	_sinst.beginTime = _time;
	// assert(_inst.sinst == _sinst);
	if(_record)
		operation_step_add(OPERATION_TYPE.ADD, _inst.get_prop(), -1);
	return _inst;
}


function build_note_withprop(prop, record = false, selecting = false) {
	var _id = variable_struct_exists(prop, "noteID") ? prop.noteID : "";
	var _subid = variable_struct_exists(prop, "subNoteID") ? prop.subNoteID : "";

	if(prop.noteType < 2) {
		return build_note(prop.noteType, prop.time, prop.position, 
			prop.width, "-1", prop.side, record, selecting, _id);
	}
	else {
		return build_hold(prop.time, prop.position, prop.width,
						  prop.time + prop.lastTime,
						  prop.side, record, selecting, _id, _subid);
	}
}

// This function can only be accessed in destroy event.
/// @param {String} noteID
function note_delete(noteID, _record = false) {
	if(objMain.deletingAllNotes) return;

	if(noteID == "null") {
		return;
	}

	var _inst = note_get_instance(noteID);
	try {
		with(objMain) {
	        var i = _inst.get_array_pos();
	        if(chartNotesArray[i].noteID == noteID) {
				DyCore_delete_note(json_stringify(chartNotesArray[i]));
	        	if(_record)
	        		operation_step_add(OPERATION_TYPE.REMOVE, 
	        						   SnapDeepCopy(chartNotesArray[i]), -1);
	            chartNotesArray[i].time = INF;
	            chartNotesArray[i].noteID = "null";
				_inst.pull_prop();
	        }
	        else {
	        	throw "Note id in array not matching instance id."
	        }
	    }
	} catch (e) {
		announcement_error($"音符删除出现错误。请将谱面导出并备份以避免问题进一步恶化。您可以选择将该信息反馈开发者以帮助我们解决问题。错误信息：{e}");
	}
    
    note_sort_request();
}

function note_delete_all() {
	with(objMain) {
		chartNotesArray = [];
		chartNotesArrayAt = 0;
		chartNotesCount = 0;
		
		note_activate_all();
		deletingAllNotes = true;
		instance_destroy(objNote);
		DyCore_clear_notes();
		deletingAllNotes = false;
	}
}

function note_delete_all_manually(_record = true) {
	note_activate_all();
	with(objNote) {
		if(noteType != 3) {
			recordRequest = _record;
			instance_destroy();
		}
	}
}

function notes_array_update() {
	with(objMain) {
		chartNotesCount = array_length(chartNotesArray);
		var i=0, l=chartNotesCount;
		for(; i<l; i++) if(chartNotesArray[i].time != INF) {
			note_get_instance(chartNotesArray[i].noteID).update_prop();
			chartNotesArray[i].index = i;
		}
	}
	note_sort_request();
}

function note_check_and_activate(_posistion_in_array) {
	var _str = objMain.chartNotesArray[_posistion_in_array];
	var _inst = note_get_instance(_str.noteID);
	if(_inst > 0)
		_str.index = _posistion_in_array;
	else
		return 0;
	if(note_is_activated(_inst)) {
		return 0;
	}
	var _note_inbound = !_outbound_check_t(_str.time, _str.side) && _str.time >= objMain.nowTime;
	var _hold_intersect = _str.noteType >= 2 &&
		(_str.noteType == 2? (_str.time <= objMain.nowTime && _str.time + _str.lastTime >= objMain.nowTime):
			(_str.beginTime <= objMain.nowTime && _str.time >= objMain.nowTime));
	if(_note_inbound || _hold_intersect || _inst.stateType != NOTE_STATES.OUT) {
		note_activate(_inst);
		return 1;
	}
	else if(!_note_inbound && _outbound_check_t(_str.time, !(_str.side))) {
		return -1;
	}
	return 0;
}

function note_deactivate(inst) {
	if(!note_is_activated(inst)) return;
	instance_deactivate_object(inst);
	global.activationMan.deactivate(inst);
}

/// @param {Id.Instance.objNote} inst 
function note_activate(inst) {
	if(note_is_activated(inst)) return;
	instance_activate_object(inst);
	inst.pull_prop();
	global.activationMan.activate(inst);
}

function note_activate_all() {
	instance_activate_object(objNote);
	global.activationMan.activate_all();
}

function note_deactivate_all() {
	with(objNote) global.activationMan.deactivate(id);
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
		ds_map_clear(deactivationQueue);
		for(var i=0; i<chartNotesCount; i++)
			note_check_and_activate(i);
	}
}

function note_generate_id() {
	return DyCore_random_string(NOTE_ID_LENGTH);
}