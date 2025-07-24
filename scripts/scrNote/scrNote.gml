
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
	stat_reset();
	var _arr = objMain.chartNotesArray
	for(var i=0, l=array_length(_arr); i<l; i++)
		if(_arr[i].time != INF) {
			stat_count(_arr[i].side, _arr[i].noteType);
		}
}

function note_sort_all() {
    var _f = function(_a, _b) {
        return sign(_a.time == _b.time ? int64(_a.inst) - int64(_b.inst) : _a.time - _b.time);
    }
    array_sort(objMain.chartNotesArray, _f);
    
    // Update arrayPos & Flush deleted notes
    with(objMain) {
    	chartNotesCount = array_length(chartNotesArray);
    	
    	for(var i=0; i<chartNotesCount; i++)
    		if(chartNotesArray[i].inst > 0)
				chartNotesArray[i].inst.arrayPos = i;
    	
    	while(array_length(chartNotesArray) > 0) {
			/// @type {Any} 
			var str = array_last(chartNotesArray);
			if(str.inst >= 0) break;
    		array_pop(chartNotesArray);
    		chartNotesCount --;
    	}
    }

	note_recac_stats();
}

function note_sort_request() {
	objEditor.editorNoteSortRequest = true;
}

function build_note(_id, _type, _time, _position, _width, 
	_subid, _side, _record = false, _selecting = false) {
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
    _inst.width = real(_width);
    _inst.side = real(_side);
	_inst.time = _time;
    _inst.position = real(_position);
    _inst.nid = _id;
    _inst.sid = _subid;
    
    with(_inst) {
    	_prop_init(true);
    	if(noteType == 2) _prop_hold_update();
    	if(_selecting) set_state(NOTE_STATES.SELECTED);
    }
    with(objMain) {
        array_push(chartNotesArray, _inst.get_prop(true));
		DyCore_insert_note(json_stringify(_inst.get_prop()));
        if(ds_map_exists(chartNotesMap[_inst.side], _id)) {
            show_error_async("Duplicate Note ID " + _id + " in side " 
                + string(_side), false);
            return -999;
        }
        chartNotesMap[_inst.side][? _id] = _inst;
        
        note_sort_request();
    }
    
    if(_record)
    	operation_step_add(OPERATION_TYPE.ADD, _inst.get_prop(), -1);
    
    return _inst;
}

function build_hold(_id, _time, _position, _width, _subid, _subtime,
					_side, _record = false, _selecting = false) {
	var _sinst = build_note(_subid, 3, _subtime, _position, _width, -1, _side,
							false, _selecting);
	var _inst = build_note(_id, 2, _time, _position, _width, _subid, 
						   _side, false, _selecting);
	_sinst.beginTime = _time;
	// assert(_inst.sinst == _sinst);
	if(_record)
		operation_step_add(OPERATION_TYPE.ADD, _inst.get_prop(), -1);
	return _inst;
}


function build_note_withprop(prop, record = false, selecting = false) {
	if(prop.noteType < 2) {
		return build_note(random_id(9), prop.noteType, prop.time, prop.position, 
			prop.width, "-1", prop.side, record, selecting);
	}
	else {
		return build_hold(random_id(9), prop.time, prop.position, prop.width,
						  random_id(9), prop.time + prop.lastTime,
						  prop.side, record, selecting);
	}
}

// This function can only be accessed in destroy event.
function note_delete(_inst, _record = false) {
	if(!instance_exists(_inst)) {
		show_debug_message("!!!Warning!!! You're trying to delete an unexisting/deactiaved note.")
		return;
	}
	if(_inst.arrayPos == -1) {
		return;
	}
	try {
		with(objMain) {
	        var i = _inst.arrayPos;
	        if(chartNotesArray[i].inst == _inst) {
	        	chartNotesArray[i] = chartNotesArray[i].inst.get_prop();
				DyCore_delete_note(json_stringify(chartNotesArray[i]));
	        	if(_record)
	        		operation_step_add(OPERATION_TYPE.REMOVE, 
	        						   SnapDeepCopy(chartNotesArray[i]), -1);
	        	
	        	ds_map_delete(chartNotesMap[chartNotesArray[i].side], _inst.nid);
	            chartNotesArray[i].time = INF;
	            chartNotesArray[i].inst = -10;
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
		ds_map_clear(chartNotesMap[0]);
		ds_map_clear(chartNotesMap[1]);
		ds_map_clear(chartNotesMap[2]);
		
		instance_activate_all();
		with(objNote) arrayPos = -1;
		instance_destroy(objNote);
		DyCore_clear_notes();
	}
}

function note_delete_all_manually(_record = true) {
	instance_activate_all();
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
			chartNotesArray[i].inst.update_prop();
			chartNotesArray[i].inst.arrayPos = i;
		}
	}
	note_sort_request();
}

function notes_reallocate_id() {
	with(objMain) {
		var i=0, l=chartNotesCount;
		ds_map_clear(chartNotesMap[0]);
		ds_map_clear(chartNotesMap[1]);
		ds_map_clear(chartNotesMap[2]);
		for(; i<l; i++) {
			var _inst = chartNotesArray[i].inst;
			_inst.nid = string(i);
			chartNotesMap[_inst.side][? _inst.nid] = _inst;
		}
		for(i=0; i<l; i++) {
			var _inst = chartNotesArray[i].inst;
			if(_inst.noteType == 2)
				_inst.sid = _inst.sinst.nid;
			else
				_inst.sid = "-1";
		}
	}
}

function note_check_and_activate(_posistion_in_array) {
	var _struct = objMain.chartNotesArray[_posistion_in_array];
	if(_struct.inst > 0)
		_struct.inst.arrayPos = _posistion_in_array;
	else
		return 0;
	if(instance_exists(_struct.inst)) {
		return 0;
	}
	var _str = _struct;
	var _note_inbound = !_outbound_check_t(_str.time, _str.side) && _str.time >= objMain.nowTime;
	var _hold_intersect = _str.noteType >= 2 &&
		(_str.noteType == 2? (_str.time <= objMain.nowTime && _str.time + _str.lastTime >= objMain.nowTime):
			(_str.beginTime <= objMain.nowTime && _str.time >= objMain.nowTime));
	if(_note_inbound || _hold_intersect || _str.inst.stateType != NOTE_STATES.OUT) {
		// instance_activate_object(_str.inst);
		note_activate(_str.inst);
		return 1;
	}
	else if(!_note_inbound && _outbound_check_t(_str.time, !(_str.side))) {
		return -1;
	}
	return 0;
}

function note_deactivate(inst) {
	instance_deactivate_object(inst);
}

function note_activate(inst) {
	instance_activate_object(inst);
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
	instance_deactivate_object(objNote);
	with(objMain) {
		ds_map_clear(deactivationQueue);
		for(var i=0; i<chartNotesCount; i++)
			note_check_and_activate(i);
	}
}

// Prevent extra sub notes.
function note_extra_sub_removal() {
	var _dcnt = 0;
	for(var i=0, l=array_length(objMain.chartNotesArray); i<l; i++)
		if(objMain.chartNotesArray[i].inst > 0) {
			/// @self Id.Instance.objNote
			with(objMain.chartNotesArray[i].inst) {
				if(noteType == 3 && finst<0) {
					instance_destroy();
					_dcnt ++;
				}
			}
		}
	if(_dcnt > 0) {
		announcement_warning(i18n_get("extra_sub_fix", string(_dcnt)));
		note_sort_all();
	}
}