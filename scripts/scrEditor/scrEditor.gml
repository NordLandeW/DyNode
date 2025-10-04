function editor_set_editmode(mode) {
	with(objEditor) {
		if(mode > 0) {
			singlePaste = false;
			operation_merge_last_request_revoke();
		}
		else {
			editorModeBeforeCopy = editorMode;
		}
		editorMode = mode;
		editorModeSwitching = 2;
	}
}

function editor_get_editmode() {
    return objEditor.editorMode;
}

function editor_mode_is_switching() {
    return objEditor.editorModeSwitching > 0;
}

function editor_get_default_width() {
	var _res = 0;
	with(objEditor) {
		switch(editorDefaultWidthMode) {
			case 0:
				_res = editorDefaultWidth[0];
				break;
			case 1:
				_res = editorDefaultWidth[1];
				if(editor_get_editside() > 0)
					_res *= 2;
				break;
			case 2:
				_res = editorDefaultWidth[2][min(editor_get_editside(), 1)];
				break;
			case 3:
				_res = editorDefaultWidth[3][editor_get_editside()];
				break;
		}
	}
	return _res;
}

function editor_set_default_width_qbox() {
	var _val = get_string_i18n("box_set_change_default_width", string(editor_get_default_width()));
	try {
		_val = real(_val);
	} catch (e) {
		if(_val != "")
			announcement_error("error_default_width_must_be_real")
	}
	if(is_real(_val)) {
		editor_set_default_width(_val);
		announcement_set("anno_default_width", string(_val));
		return true;
	}
	return false;
}

function editor_set_default_width(width) {
	with(objEditor) {
		switch(editorDefaultWidthMode) {
			case 0:
				editorDefaultWidth[0] = width;
				break;
			case 1:
				if(editor_get_editside() > 0)
					width /= 2;
				editorDefaultWidth[1] = width;
				break;
			case 2:
				editorDefaultWidth[2][min(editor_get_editside(), 1)] = width;
				break;
			case 3:
				editorDefaultWidth[3][editor_get_editside()] = width;
				break;
		}
	}
}

function editor_set_editside(side, same_then_silence = false) {
	var _sidename = ["editside_down", "editside_left", "editside_right", "editside_LR"];

	if(side < 3) {
		if(!(same_then_silence && objEditor.editorSide == side))
			announcement_play(i18n_get("anno_editside_switch") + ": " +i18n_get(_sidename[side]));
		objEditor.editorSide = side;
		editor_lrside_set(false);
	}
	else {
		if(!(same_then_silence && editor_lrside_get()))
			announcement_play(i18n_get("anno_editside_switch") + ": " +i18n_get(_sidename[side]));
		editor_lrside_set(true);
	}
	
	if(editor_get_editmode() == 5)
		editor_set_editmode(4);
}

function editor_get_editside() {
    return objEditor.editorSide;
}

function editor_select_is_going() {
	return objEditor.editorSelectOccupied;
}
function editor_select_is_multiple() {
	return objEditor.editorSelectMultiple;
}
function editor_select_is_dragging() {
	return objEditor.editorSelectDragOccupied;
}
function editor_select_is_area() {
	return objEditor.editorSelectArea;
}
function editor_editside_allowed(side) {
	if(objEditor.editorLRSide && side > 0)
		return true;
	return side == editor_get_editside();
}
function editor_lrside_set(enable) {
	with(objEditor) {
		editorLRSide = enable;
		editorLRSideLock = false;
	}
}
function editor_lrside_get() {
	return objEditor.editorLRSide;
}
function editor_lrside_lock_set(lock) {
	objEditor.editorLRSideLock = lock;
}
function editor_select_get_area_position() {
	var _pos;
	with(objEditor) {
		_pos = noteprop_to_xy(editorSelectAreaPosition.pos, editorSelectAreaPosition.time, editorSide);
		_pos[2] = mouse_x;
		_pos[3] = mouse_y;
	}
	return _pos;
}
function editor_select_inbound(x, y, side, type, onlytime = -1) {
	var _pos = editor_select_get_area_position();
	return editor_editside_allowed(side) && type != 3 && pos_inbound(x, y, _pos[0], _pos[1], _pos[2], _pos[3], onlytime)
}

function editor_select_count() {
	return objEditor.editorSelectCount;
}

function editor_select_reset() {
	objEditor.editorSelectResetRequest = true;
}

function editor_select_all() {
	if(editor_get_editmode() != 4) return;

	with(objNote) {
		set_state(NOTE_STATES.SELECTED);
	}
}

function editor_snap_to_grid_time(_time, _side, _ignore_boundary = false) {
	return editor_snap_to_grid_y(note_time_to_y(_time, _side), _side, _ignore_boundary);
}

function editor_snap_to_grid_y(_y, _side, _ignore_boundary = false) {
    var _nw = BASE_RES_W, _nh = BASE_RES_H;
    
    var _time = y_to_note_time(_y, _side);
    var _nowat = 0;
	var _ret = {
	    	y: _y,
			time: _time,
	    	bar: undefined
    	};
	
	var timingPoints = dyc_get_timingpoints();
	if(!objEditor.editorGridYEnabled || !array_length(timingPoints)) return _ret;

    with(objEditor) {
        var targetLineBelow = objMain.targetLineBelow;
        var targetLineBeside = objMain.targetLineBeside;
        var playbackSpeed = objMain.playbackSpeed;
        var _l = array_length(timingPoints);
        var _totalBar = 1;
        while(_nowat + 1 != _l && timingPoints[_nowat+1].time <= _time) {
        	_totalBar += ceil((timingPoints[_nowat+1].time - timingPoints[_nowat].time)
        		/(timingPoints[_nowat].beatLength*timingPoints[_nowat].meter));
        	_nowat ++;
        }
        var _nowtp = timingPoints[_nowat];
        var _nowbeats = floor((_time - _nowtp.time) / _nowtp.beatLength);
        var _nexttime = (_nowat + 1 == _l ? objMain.musicLength:timingPoints[_nowat+1].time)
        var _nowdivb = objEditor.get_div(); // divs per beat
        var _nowdiv = 1 / _nowdivb * _nowtp.beatLength;
        var _nowdivbm = _nowdivb * _nowtp.meter; // divs per bar
        
        var _ntime = (_time - _nowbeats * _nowtp.beatLength - _nowtp.time) / _nowdiv;
        var _rd = round(_ntime); // now divs in one beat
        var _rt = _rd * _nowdiv;
        var _rbd = round(_ntime)==ceil(_ntime) ? floor(_ntime) : ceil(_ntime);
        var _rbt = _rbd * _nowdiv;
        _rt += _nowbeats * _nowtp.beatLength + _nowtp.time;
        _rbt += _nowbeats * _nowtp.beatLength + _nowtp.time;
        var _ry = note_time_to_y(_rt, min(_side, 1));
        var _rby = note_time_to_y(_rbt, min(_side, 1));
        
        var _eps = 1;	// Prevent some precision problems
        var _f_genret = method({
        	_nowbeats: _nowbeats,
        	_nowdivb: _nowdivb,
        	_nowdivbm: _nowdivbm,
        	_totalBar: _totalBar
        }, function (_ny, _d, _t) {
        	return {
        		y: _ny,
            	bar: floor((_d + _nowbeats * _nowdivb)/_nowdivbm) + _totalBar,
            	diva: ((_d + _nowbeats * _nowdivb) % _nowdivbm + _nowdivbm) % _nowdivbm,
            	divb: _nowdivbm,
            	divc: _nowdivb * 4,
				time: _t
        	};
        });
        
        if(_side == 0) {
            if((in_between(_ry, 0, _nh - targetLineBelow) || _ignore_boundary) && _rt + _eps <= _nexttime)
                _ret = _f_genret(_ry, _rd, _rt);
            else if((in_between(_rby, 0, _nh - targetLineBelow) || _ignore_boundary) && _rbt + _eps <= _nexttime)
                _ret = _f_genret(_rby, _rbd, _rbt);
        }
        else {
            if((in_between(_ry, targetLineBeside, _nw/2) || _ignore_boundary) && _rt + _eps <= _nexttime)
                _ret = _f_genret(note_time_to_y(_rt, _side), _rd, _rt);
            else if((in_between(_rby, targetLineBeside, _nw/2) || _ignore_boundary) && _rbt + _eps <= _nexttime)
                _ret = _f_genret(note_time_to_y(_rbt, _side), _rbd, _rbt);
        }
    }
    
    return _ret;
}

function editor_snap_to_grid_x(_x, _side) {
	if(!objEditor.editorGridXEnabled) return _x;
	
	var _pos = x_to_note_pos(_x, _side);
	_pos = round(_pos * 10) / 10;
	return note_pos_to_x(_pos, _side);
}

function editor_snap_width(_width) {
	if(objEditor.editorGridWidthEnabled)
		_width = round(_width * 20) / 20; // per 0.05
	return _width;
}

// Comparison function to deal with multiple single selection targets
function editor_select_compare(ida, idb) {
	if(!instance_exists(ida)) return idb;
	else if(!instance_exists(idb)) return ida;
	else if(ida.priority < idb.priority) return ida;
	else if(ida.priority > idb.priority) return idb;
	else if(ida.time < idb.time) return ida;
	else if(ida.time > idb.time) return idb;
	else return min(ida, idb);
}

function note_build_attach(_type, _side, _width, _pos=0, _time=0, _lasttime = -1) {
    var _obj = [objNote, objChain, objHold];
    _obj = _obj[_type];
    
	/// @type {Id.Instance.objNote} 
    var _inst = instance_create_depth(mouse_x, mouse_y, 
                0, _obj);
    
    with(_inst) {
		set_state(NOTE_STATES.ATTACH);
        width = _width;
        side = _side;
        fixedLastTime = _lasttime;
        origPosition = _pos;
        origTime = _time;
        attaching = true;
        _prop_init(true);
        
        if(_lasttime != -1 && _type == 2) {
        	sinst = instance_create(x, y, objHoldSub);
        	sinst.dummy = true;
        	_prop_hold_update(false);
        }
    }
    
    return _inst;
}

/// @returns {Id.Instance.objNote} Note instance.
function editor_get_note_attaching_center() {
	return objEditor.editorNoteAttaching[objEditor.editorNoteAttachingCenter];
}

/// @description Target at all selected notes, duplicate them to the next divided beat.
function editor_note_duplicate_quick() {
	if(timing_point_count() == 0) return;
	var maxTime = -10000000;
	var minTime = 0x7fffffff;
	with(objNote) {
		if(stateType == NOTE_STATES.SELECTED) {
			maxTime = max(maxTime, time + lastTime);
			minTime = min(minTime, time);
		}
	}
	var spacing = maxTime - minTime + timing_point_get_at(maxTime).beatLength / editor_get_div();
	// Spacing snapping correction
	spacing = editor_snap_to_grid_time(minTime + spacing, 0, true).time - minTime;
	with(objNote) {
		if(stateType == NOTE_STATES.SELECTED) if(noteType != 3) {
			note_select_reset(id);
			var _prop = get_prop();
			_prop.time += spacing;
			build_note(_prop, true, true, true);
			operation_merge_last_request(1, OPERATION_TYPE.DUPLICATE);
		}
	}
	objMain.time_range_made_inbound(minTime + spacing, maxTime + spacing);
}

#region UNDO & REDO FUNCTION

function operation_get_name(opsType) {
	var _result = "";
	switch(opsType) {
		case OPERATION_TYPE.MOVE:
			_result = "ops_name_move";
			break;
		case OPERATION_TYPE.ADD:
			_result = "ops_name_add";
			break;
		case OPERATION_TYPE.REMOVE:
			_result = "ops_name_remove";
			break;
		case OPERATION_TYPE.TPADD:
			_result = "ops_name_tpadd";
			break;
		case OPERATION_TYPE.TPCHANGE:
			_result = "ops_name_tpchange";
			break;
		case OPERATION_TYPE.TPREMOVE:
			_result = "ops_name_tpremove";
			break;
		case OPERATION_TYPE.OFFSET:
			_result = "ops_name_offset";
			break;
		case OPERATION_TYPE.ATTACH:
			_result = "ops_name_attach";
			break;
		case OPERATION_TYPE.CUT:
			_result = "ops_name_cut";
			break;
		case OPERATION_TYPE.MIRROR:
			_result = "ops_name_mirror";
			break;
		case OPERATION_TYPE.ROTATE:
			_result = "ops_name_rotate";
			break;
		case OPERATION_TYPE.PASTE:
			_result = "ops_name_paste";
			break;
		case OPERATION_TYPE.SETTYPE:
			_result = "ops_name_settype";
			break;
		case OPERATION_TYPE.SETWIDTH:
			_result = "ops_name_setwidth";
			break;
		case OPERATION_TYPE.RANDOMIZE:
			_result = "ops_name_randomize";
			break;
		case OPERATION_TYPE.DUPLICATE:
			_result = "ops_name_duplicate";
			break;
		case OPERATION_TYPE.EXPR:
			_result = "ops_name_expr";
			break;
	}
	return i18n_get(_result);
}

function operation_synctime_set(time) {
	with(objEditor) {
		operationSyncTime[0] = min(operationSyncTime[0], time);
		operationSyncTime[1] = max(operationSyncTime[1], time);
	}
}
function operation_synctime_sync() {
	if(objEditor.operationSyncTime[0] == INF) return;
	var _time = objEditor.operationSyncTime;
	
	objMain.time_range_made_inbound(_time[0], _time[1], 300);
	objEditor.operationSyncTime = [INF, -INF];
}

function operation_step_add(_type, _from, _to) {
	with(objEditor) {
		array_push(operationStackStep, new sOperation(_type, _from, _to));
	}
}

/// @param {Struct.sOperation} _array 
function operation_step_flush(_array) {
	with(objEditor) {
		var _operation_pack = {
			type: _array[0].opType,
			ops: _array
		}

		array_resize(operationStack, operationPointer + 1);
		array_push(operationStack, _operation_pack);
		operationPointer ++;
		operationCount = operationPointer + 1;
		// show_debug_message_safe($"New operation: {_array}");

		if(operationMergeLastRequest > 0) {
			operationMergeLastRequestCount ++;
			if(operationMergeLastRequest == operationMergeLastRequestCount) {
				operation_merge_last(operationMergeLastRequest, operationMergeLastRequestType);
				operationMergeLastRequest = 0;
				operationMergeLastRequestCount = 0;
			}
		}
	}
}

/// @param {Any} _to 
function operation_do(_type, _from, _to = -1, _safe_ = false) {
	if(_to != -1)
		operation_synctime_set(_to.time);
	else if(is_struct(_from))
		operation_synctime_set(_from.time);
	switch(_type) {
		case OPERATION_TYPE.ADD:
			return build_note(_from, false, true);
			break;
		case OPERATION_TYPE.MOVE:
			if(!is_struct(_to)) {
				announcement_error("Error in operation_do (MOVE): target property is not a struct.");
				return;
			}
			dyc_update_note(_to);
			if(!_safe_) {
				note_activate(_to.noteID);
				var _inst = note_get_instance(_to.noteID);
				_inst.select();
				_inst.pull_prop();
			}
			break;
		case OPERATION_TYPE.REMOVE:
			note_delete(_from.noteID, false);
			break;
		case OPERATION_TYPE.TPADD:
			timing_point_add(_from.time, _from.beatLength, _from.meter);
			break;
		case OPERATION_TYPE.TPREMOVE:
			timing_point_delete_at(_from.time);
			break;
		case OPERATION_TYPE.TPCHANGE:
			var _tp = timing_point_get_at(_from.time);
			_tp.time = _to.time;
			_tp.beatLength = _to.beatLength;
			_tp.meter = _to.meter;
			dyc_timingpoints_change(_tp.time, _tp);
			break;
		case OPERATION_TYPE.OFFSET:
			map_add_offset(_from);
			break;
	}
}

function operation_undo() {
	with(objEditor) {
		if(operationPointer == -1) return;
		var _ops = operationStack[operationPointer].ops;
		var _type = operationStack[operationPointer].type;
		
		note_select_reset();
		for(var i=0, l=array_length(_ops); i<l; i++) {
			switch(_ops[i].opType) {
				case OPERATION_TYPE.MOVE:
					operation_do(OPERATION_TYPE.MOVE, _ops[i].toProp, _ops[i].fromProp, l > MAX_SELECTION_LIMIT);
					break;
				case OPERATION_TYPE.ADD:
					operation_do(OPERATION_TYPE.REMOVE, _ops[i].fromProp);
					break;
				case OPERATION_TYPE.REMOVE:
					var _inst = operation_do(OPERATION_TYPE.ADD, _ops[i].fromProp);
					break;
				case OPERATION_TYPE.TPADD:
					operation_do(OPERATION_TYPE.TPREMOVE, _ops[i].fromProp);
					break;
				case OPERATION_TYPE.TPREMOVE:
					operation_do(OPERATION_TYPE.TPADD, _ops[i].fromProp);
					break;
				case OPERATION_TYPE.TPCHANGE:
					operation_do(OPERATION_TYPE.TPCHANGE, _ops[i].toProp, _ops[i].fromProp);
					break;
				case OPERATION_TYPE.OFFSET:
					operation_do(OPERATION_TYPE.OFFSET, -_ops[i].fromProp);
					break;
				default:
					show_error("Unknown operation type.", true);
			}
		}
		
		operationPointer--;
		
		announcement_play(i18n_get("undo", [operation_get_name(_type), string(array_length(_ops))]));
		note_sort_request();
	}
	
}

function operation_redo() {
	with(objEditor) {
		if(operationPointer + 1 == operationCount) return;
		operationPointer ++;
		var _ops = operationStack[operationPointer].ops;
		var _type = operationStack[operationPointer].type;
		note_select_reset();
		for(var i=0, l=array_length(_ops); i<l; i++) {
			switch(_ops[i].opType) {
				case OPERATION_TYPE.MOVE:
				case OPERATION_TYPE.TPCHANGE:
					operation_do(_ops[i].opType, _ops[i].fromProp, _ops[i].toProp, l > MAX_SELECTION_LIMIT);
					break;
				case OPERATION_TYPE.ADD:
					var _inst = operation_do(OPERATION_TYPE.ADD, _ops[i].fromProp);
					break;
				case OPERATION_TYPE.REMOVE:
				case OPERATION_TYPE.TPADD:
				case OPERATION_TYPE.TPREMOVE:
				case OPERATION_TYPE.OFFSET:
					operation_do(_ops[i].opType, _ops[i].fromProp);
					break;
				default:
					show_error("Unknown operation type.", true);
			}
		}

		announcement_play(i18n_get("redo", [operation_get_name(_type), string(array_length(_ops))]));
		note_sort_request();
	}
}

/// @description Merge last operations to one operation.
/// @param {Real} count The number of last operations to merge.
/// @param {Enum.OPERATION_TYPE} type The type of the merged operations.
function operation_merge_last(count, type) {
	with(objEditor) {
		var _new_ops = [];
		for(var i=0; i<count; i++) {
			_new_ops = array_concat(_new_ops, operationStack[operationPointer - i].ops);
		}
		operationPointer -= count - 1;
		operationCount = operationPointer + 1;
		operationStack[operationPointer] = {
			type: type,
			ops: _new_ops
		};
	}
}

/// @description Send requests to merge the last new operations from now on.
/// @param {Real} count The number of last operations to merge.
/// @param {Enum.OPERATION_TYPE} type The type of the merged operations.
function operation_merge_last_request(count, type) {
	with(objEditor) {
		operationMergeLastRequestCount = 0;
		operationMergeLastRequest = count;
		operationMergeLastRequestType = type;
	}
}

/// @description Revoke the merge operations request.
function operation_merge_last_request_revoke() {
	objEditor.operationMergeLastRequest = 0;
	objEditor.operationMergeLastRequestCount = 0;
}

#endregion

#region TIMING POINT FUNCTION

// The number of timing points' count.
function timing_point_count() {
	return dyc_get_timingpoints_count();
}

// Sort the "timingPoints" array
function timing_point_sort() {
    dyc_timingpoints_sort();
}

// Add a timing point to "timingPoints" array
function timing_point_add(_t, _l, _b, record = false) {
	if(dyc_has_timing_point_at_time(_t)) {
		announcement_error(i18n_get("timing_duplicate_error", [_t]));
		return false;
	}


    var _tp = new sTimingPoint(_t, _l, _b);
    dyc_insert_timingpoint(_tp);
    timing_point_sort();

    if(record)
    	operation_step_add(OPERATION_TYPE.TPADD, _tp, -1);
	
	return true;
}

function timing_point_create(record = false) {
	var _time = objMain.nowTime;
	if(editor_select_count() == 1) {
		var _ntime = 0;
		with(objNote)
			if(stateType == NOTE_STATES.SELECTED)
				_ntime = time;
		var _ptp = timing_point_get_at(_ntime, true);
		if(_ptp != undefined) {
			timing_point_change(_ptp, record);
			return;
		}
		_time = _ntime;
	}
	_time = string_digits(get_string_i18n("tpc_q1", string_format(_time, 1, 0)));
	if(_time == "") return;
	var _bpm = string_real(get_string_i18n("tpc_q2", ""));
	if(_bpm == "") return;
	var _meter = string_digits(get_string_i18n("tpc_q3", ""));
	if(_meter == "") return;
	
	_time = real(_time);
	_bpm = real(_bpm);
	_meter = real(_meter);
	
	_bpm = bpm_to_mspb(_bpm);

	if(timing_point_add(_time, _bpm, _meter, record))
		announcement_play(
			i18n_get("add_timing_point", [format_time_ms(_time), string(mspb_to_bpm(_bpm)), string(_meter)]), 
			5000);
}

function timing_point_change(tp, record = false) {
	var _origTime = tp.time;
	var _current_setting = $"{tp.time} , {mspb_to_bpm(tp.beatLength)} , {tp.meter}";
	var _setting = dyc_get_string(i18n_get("timing_point_change", tp.time), _current_setting);
	if(_setting == _current_setting || _setting == "")
		return;
	try {
		var _arr = string_split(_setting, ",", true);
		var _oarr = string_split(_current_setting, ",", true);
		var _noffset = real(_arr[0]);
		var _nbpm = real(_arr[1]);
		var _nmeter = int64(_arr[2]);

		if(_noffset != _origTime && dyc_has_timing_point_at_time(_noffset)) {
			announcement_error(i18n_get("timing_duplicate_error", [_noffset]));
			return;
		}

		var _tpBefore = SnapDeepCopy(tp);
		var _tpAfter = SnapDeepCopy(tp);
		var _fixable = false;
		if(_oarr[0] != _arr[0]) {
			_tpAfter.time = _noffset;
			_fixable = true;
		}
		if(_oarr[1] != _arr[1]) {
			_tpAfter.beatLength = bpm_to_mspb(_nbpm);
			_fixable = true;
		}
		if(_oarr[2] != _arr[2])
			_tpAfter.meter = _nmeter;

		if(_fixable)
			timing_fix(tp, _tpAfter);

        dyc_timingpoints_change(tp.time, _tpAfter);

		if(record)
			operation_step_add(OPERATION_TYPE.TPCHANGE, _tpBefore, _tpAfter);
		
		timing_point_sort();
		announcement_play(i18n_get("timing_point_change_success", [tp.time, _nbpm, _nmeter]), 5000);
	} catch (e) {
		announcement_error(i18n_get("timing_point_change_err") + "\n[scale,0.5]" + string(e));
		return;
	}
}

function timing_fix(tpBefore, tpAfter) {
	var timingPoints = dyc_get_timingpoints();
	var l = array_length(timingPoints);
	var at = -1;
	for(var i=0; i<l; i++)
		if(timingPoints[i].time == tpBefore.time) {
			at = i;
			break;
		}
	// Get affected time range.
	var _timeL = tpBefore.time, _timeR = at+1 == l? 1000000000: timingPoints[at+1].time - 1;
	var _timeM = at - 1 < 0 ? -1000000000: timingPoints[at-1].time;
	var nl = DyCore_get_note_count();
	// Get affected notes.
	var _affectedNotes = [];
	
	var _cross_timing_warning = false;
	for(var i=0; i<nl; i++)
		if(in_between(dyc_get_note_time_at_index(i), _timeL, _timeR))
			array_push(_affectedNotes, dyc_get_note_at_index(i));
	if(array_length(_affectedNotes) == 0)
		return;
	var _que = dyc_show_question(i18n_get("timing_fix_question", [_timeL, at+1 == l?objMain.musicLength:_timeR, array_length(_affectedNotes)]));
	if(!_que) return;
	nl = array_length(_affectedNotes);
	// Caculate note's new time.
	for(var i=0; i<nl; i++) {
		/// @type {Struct.sNoteProp} 
		var _prop = SnapDeepCopy(_affectedNotes[i]);
		if(_prop.noteType == NOTE_TYPE.SUB) continue;
		_prop.time = (_prop.time - tpBefore.time) * (tpAfter.beatLength / tpBefore.beatLength) + tpAfter.time;
		if(_prop.noteType == NOTE_TYPE.HOLD)
			_prop.lastTime *= (tpAfter.beatLength / tpBefore.beatLength);
		if(_prop.time > _timeR)
			_cross_timing_warning = true;
		dyc_update_note(_prop, true);
	}
	if(tpAfter.time < _timeM)
		_cross_timing_warning = true;	// Timing's offset conflicts with another timing.
	note_sort_request();
	if(_cross_timing_warning)
		announcement_warning("timing_fix_cross_warning");
}

// To get current timing at [_time].
/// @returns {Struct.sTimingPoint} 
function timing_point_get_at(_time, _precise = false) {
    var timingPoints = dyc_get_timingpoints();
	if(array_length(timingPoints) == 0) return undefined;
	/// @self Struct.sTimingPoint
	var _ret = timingPoints[
		max(array_upper_bound(
			timingPoints,
			_time,
			function(array, at) { return array[at].time; }) - 1, 0)
		];
		
	if(!_precise || abs(_time - _ret.time) < 5)
		return _ret;
	else
		return undefined;
}

function timing_point_delete_at(_time, record = false) {
    var timingPoints = dyc_get_timingpoints();
	for(var i=0, l=array_length(timingPoints); i<l; i++)
		if(abs(timingPoints[i].time-_time) <= 1) {
			var _tp = timingPoints[i];
			announcement_play(
				i18n_get("remove_timing_point", [format_time_ms(_tp.time),
    				string(mspb_to_bpm(_tp.beatLength)), string(_tp.meter)]),
				5000);
    			
    		if(record)
    			operation_step_add(OPERATION_TYPE.TPREMOVE, _tp, -1);
            
            dyc_timingpoints_delete_at(_tp.time);
		}
}

// Duplicate the last timing point at certain point
function timing_point_duplicate(_time) {
    var timingPoints = dyc_get_timingpoints();
	if(array_length(timingPoints) == 0) {
		announcement_error("error_no_timing_point");
		return;
	}
	var _tp = timingPoints[array_length(timingPoints) - 1];
	timing_point_add(_time, _tp.beatLength, _tp.meter, true);
	
	announcement_play(
		i18n_get("copy_timing_point", [format_time_ms(_time), 
			string(mspb_to_bpm(_tp.beatLength)), string(_tp.meter)]), 
		5000);
}

// Reset the "timingPoints" array
function timing_point_reset() {
    dyc_timingpoints_reset();
}

#endregion

/// surprise
function chart_randomize() {
	for(var i=0, l=dyc_get_note_count(); i<l; i++) {
		var _str = dyc_get_note_at_index_direct(i);
		if(_str.noteType == 3) continue;
		var origProp = SnapDeepCopy(_str);
		_str.position = dyc_random(5);
		_str.side = dyc_irandom_range(0, 2);
		_str.width = dyc_random_range(0.5, 5);
		operation_step_add(OPERATION_TYPE.MOVE, origProp, _str);
		dyc_update_note(_str);
	}
	operation_merge_last_request(1, OPERATION_TYPE.RANDOMIZE);
	note_sort_all(true);
}

/// For advanced property modifications.
function advanced_expr() {
	with(objEditor) {
		var _global = editorSelectCount == 0;
		var _scope_str = _global?"你正在对谱面的所有音符进行高级操作。":"你正在对选定的音符进行高级操作。";
		var _expr = dyc_get_string(_scope_str+"请填写表达式：", editorLastExpr);
		if(_expr == "") return;
		var _using_bar = string_last_pos(_expr, "bar");
		var _success = true;
		var _exec = function(_noteProp, _expr) {
			if(_noteProp.noteType != 3) {
				/// @type {Any} 
				var _prop = SnapDeepCopy(_noteProp);
				var _nprop = SnapDeepCopy(_noteProp);
				
				expr_init(); // Reset symbol table
				expr_set_var("time", _prop.time);
				expr_set_var("pos", _prop.position);
				expr_set_var("wid", _prop.width);
				expr_set_var("len", _prop.lastTime);
				expr_set_var("htime", _prop.time);
				expr_set_var("etime", _prop.time + _prop.lastTime);
				
				var _result = expr_exec(_expr);

				if(!_result) {
					announcement_error("advanced_expr_error");
					return false;
				}
				
				_nprop.time = expr_get_var("time");
				_nprop.position = expr_get_var("pos");
				_nprop.width = expr_get_var("wid");
				if(_noteProp.noteType == 2) {
					if(expr_get_var("htime") != _prop.time) {
						_nprop.lastTime = _prop.lastTime - (expr_get_var("htime") - _prop.time);
						_nprop.time = expr_get_var("htime");
					}
					if(expr_get_var("len") != _prop.lastTime)
						_nprop.lastTime = expr_get_var("len");
					else if(expr_get_var("etime") != _prop.lastTime + _prop.time)
						_nprop.lastTime = expr_get_var("etime") - _nprop.time;
				}
				
				dyc_update_note(_nprop, true);
				
				delete _prop;
				delete _nprop;
				return true;
			}
			return true;
		}
		
		if(_global) {
			for(var i=0, l=DyCore_get_note_count(); i<l; i++) {
				var _note = dyc_get_note_at_index_direct(i);
				_success = _success && _exec(_note, _expr);
				if(!_success) break;
			}
		}
		else {
			with(objNote) {
				if(stateType == NOTE_STATES.SELECTED) {
					_success = _success && _exec(get_prop(), _expr);
					pull_prop();
					if(!_success) return;
				}
			}
		}
		
		if(_success)
			announcement_play("表达式执行成功。");
		else {
			announcement_error("表达式执行失败。");
		}
			
		editorLastExpr = _expr;

		operation_merge_last_request(1, OPERATION_TYPE.EXPR);
		note_sort_all(true);
	}
}

// Advanced divisor setter
function editor_set_div() {
	var _div = get_string_i18n("box_set_div", string(editor_get_div()));
	if(_div == "") return 0;
	try {
		_div = int64(_div);
		if(_div<1) {
			throw "Bad range.";
		}
		else {
			objEditor.set_div(_div);
		}
	} catch (e) {
		announcement_error("Please input a valid number.");
		return -1;
	}
	return 1;
}

function editor_get_div() {
	return objEditor.get_div();
}

// error correction
function note_error_correction(_limit, _array, _sync_to_instance = true) {
	if(_limit <= 0) {
		announcement_error($"不合法的修正参数{_limit}。请使用大于零的误差。");
		return;
	}

	var notes_to_fix = [];
	for(var i=0, l=array_length(_array); i < l; i++) {
		if(i==0) {
			array_push(notes_to_fix, _array[i]);
		}
		else {
			assert(_array[i].time >= _array[i-1].time);

			if(_array[i].time - notes_to_fix[0].time <= _limit)
				array_push(notes_to_fix, _array[i]);
			else {
				if(array_length(notes_to_fix) > 1) {
					for(var _i=1, _l=array_length(notes_to_fix); _i < _l; _i++) {
						notes_to_fix[_i].time = notes_to_fix[0].time;
						if(_sync_to_instance)
							dyc_update_note(notes_to_fix[_i], true);
					}
				}
				notes_to_fix = [_array[i]];
			}
		}
	}

	if(array_length(notes_to_fix) > 1) {
		for(var _i=1, _l=array_length(notes_to_fix); _i < _l; _i++) {
			notes_to_fix[_i].time = notes_to_fix[0].time;
			if(_sync_to_instance)
				dyc_update_note(notes_to_fix[_i], true);
		}
	}
}

function note_outbound_warning() {
	announcement_warning("warning_note_outbound", 5000, "wob");
}
