
global.activationMan.track(id);

enum NOTE_STATES {
    OUT,
    NORMAL,
    SELECTED,
    LAST,
    ATTACH,
    DROP,
    ATTACH_SUB,
    DROP_SUB,
}

depth = 0;
drawVisible = false;
priority = int64(-10000000);
priorityRandomSeed = dyc_random(2);
image_yscale = 1;

// In-Variables

    noteID = "";
    subNoteID = "";
    stateType = NOTE_STATES.OUT;
    sprite = sprNote2;
    width = 2.0;
    position = 2.5;
    side = 0;
    bar = 0;
    time = 0;
    /// @type {Id.Instance.objHoldSub} 
    sinst = -999;					// Sub instance id
    /// @type {Id.Instance.objHold} 
    finst = -999;					// Father instance id
    noteType = 0;					// 0 Note 1 Chain 2 Hold
    /// @type {Struct.sNoteProp} Note property structure
    propertyStr = undefined;
    
    // For Editor
    origWidth = width;
    origTime = time;
    origPosition = position;
    origY = y;
    origX = x;
    origLength = 0;					// For hold
    origSubTime = 0;				// For hold's sub
    origProp = -1;					// For Undo & Redo
    origPropWidthAdjust = -1;       // For Width Adjust Operations' Undo & Redo
    origSide = side;                // For special LR mode
    fixedLastTime = -1; 			// For hold's copy and paste
    isDragging = false;
    nodeRadius = 22;				// in Pixels
    nodeColor = c_blue;
    
    // For Hold & Sub
    lastTime = 0;                   // hold's length
    beginTime = 999999999;
    lastAlphaL = 0;
    lastAlphaR = 1;
    lastAlpha = lastAlphaL;
    
    pWidth = (width * 300 - 30)*2; // Width In Pixels
    originalWidth = sprite_get_width(sprite);
    
    // For edit
    selected = false;
    selBlendColor = 0x4fd5ff;
    nodeAlpha = 0;                  // For colored square
    nodeBorderAlpha = 0;            // For colored square
    infoAlpha = 0;
    animTargetNodeA = 0;            // For colored square
    animTargetNodeBorderA = 0;      // For colored square
    animTargetInfoA = 0;            // For colored square
    unselectHint = false;           // For colored square
    recordRequest = false;
    selectInbound = false;			// If time inbound multi selection
    selectUnlock = false;			// If the state in last step is select
    selectTolerance = false;		// Make the notes display normally when being or might be selected
    attaching = false;				// If is a attaching note
    /// @type {Any} 
    lastAttachBar = -1;
    dropWidthError = global.dropAdjustError;
    dropWidthAdjustable = false;
    
    animSpeed = 0.4;
    animPlaySpeedMul = 1;
    animTargetA = 1;
    animTargetLstA = lastAlpha;
    image_alpha = editor_mode_is_switching()?1:0;
    
    // Particles Varaiables
#macro PARTICLE_HOLD_DELAY (33)     // in ms
#macro PARTICLE_NOTE_NUMBER (6)
#macro PARTICLE_NOTE_LAST (1)
    partHoldTimer = 0;
    
    dFromBottom = 0;
    uFromTop = 0;

// In-Functions

    function _get_subnote_id() {
        return subNoteID;
    }

    /// @description Initialize the note's properties using internal values.
    function _prop_init(forced = false) {
        if(editor_get_editmode() != 5 || forced) {
            priority = -20000000;
            if(noteType == 1) priority = priority * 2;
            else if(noteType == 2) priority = priority / 2;
            if(side != 0) priority += 5000000;
            
            if(attaching) priority = -100000000;
            originalWidth = sprite_get_width(sprite);
            
            // Properties Limitation
            width = max(width, 0.01);
            
            pWidth = note_biased_width(width, side) + _note_get_lrpadding_total(noteType);
            pWidth = max(pWidth, originalWidth);
            image_xscale = pWidth / originalWidth;
            image_angle = (side == 0 ? 0 : (side == 1 ? 270 : 90));
            priority = priority - time * 16 - priorityRandomSeed;
            if(noteType == 3 && note_exists(finst))
                priority = finst.priority;
        }
        
        noteprop_set_xy(position, time, side);
    }
    /// @description Update the hold's sub note properties. Including `update_prop()`
    /// @param {Bool} sync_to_array If set to true, calls `update_prop()` on sinst and self.
    _prop_hold_update = function(sync_to_array = true) {}   // Place holder.

    function _mouse_inbound_check(_mode = 0) {
        var _bbox = get_bbox();

        switch _mode {
            case 0:
                return mouse_inbound(_bbox.left, _bbox.top, _bbox.right, _bbox.bottom);
            case 1:
                return mouse_inbound_last_l(_bbox.left, _bbox.top, _bbox.right, _bbox.bottom);
            case 2:
                return mouse_inbound_last_double_l(_bbox.left, _bbox.top, _bbox.right, _bbox.bottom);
        }

        delete _bbox;
    }

    function get_array_pos() {
        return dyc_get_note_array_index(noteID);
    }

    /// @description Get the note's property structure.
    function get_prop(_set_pointer = false) {
    	var _prop = new sNoteProp({
        	time : time,
        	side : side,
        	width : width,
        	position : position,
        	lastTime : lastTime,
        	noteType : noteType,
        	noteID : noteID,
        	subNoteID: subNoteID,
        	beginTime : beginTime
        });
    	if(_set_pointer) {
    		propertyStr = _prop;
    	}
    	return _prop;
    }

    function set_prop(prop, record = false) {
        prop.noteID = noteID;
        dyc_update_note(prop, record);
        pull_prop();
    }

    /// @returns {Any} 
    function get_bbox() {
        var wid = pWidth;
        if(side == 0) {
            return {
                left: x - wid / 2,
                right: x + wid / 2,
                top: bbox_top,
                bottom: bbox_bottom
            }
        }
        else {
            return {
                left: bbox_left,
                right: bbox_right,
                top: y - wid / 2,
                bottom: y + wid / 2
            }
        }
    }
    
    /// @description Push the note's property structure and sync to backend.
    function update_prop() {
        if(!is_struct(propertyStr)) {
            return;
        }
        var differs = false;
        if(propertyStr.time != time) {
            propertyStr.time = time;
            differs = true;
        }
        if(propertyStr.side != side) {
            propertyStr.side = side;
            differs = true;
        }
        if(propertyStr.width != width) {
            propertyStr.width = width;
            differs = true;
        }
        if(propertyStr.position != position) {
            propertyStr.position = position;
            differs = true;
        }
        if(propertyStr.lastTime != lastTime) {
            propertyStr.lastTime = lastTime;
            differs = true;
        }
        if(propertyStr.noteType != noteType) {
            propertyStr.noteType = noteType;
            differs = true;
        }
        if(propertyStr.beginTime != beginTime) {
            propertyStr.beginTime = beginTime;
            differs = true;
        }

        if(differs) {
            dyc_update_note(propertyStr);
            if(note_exists(finst))
                finst._prop_hold_update(true);
            if(note_exists(sinst))
                _prop_hold_update(true);
        }
    }

    function can_pull() {
        var _selected = stateType == NOTE_STATES.SELECTED;
        if(noteType == 2)
            _selected = _selected || (note_exists(sinst) && sinst.stateType == NOTE_STATES.SELECTED);
        if(noteType == 3)
            _selected = _selected || (note_exists(finst) && finst.stateType == NOTE_STATES.SELECTED);
        return !(_selected && editor_select_is_dragging());
    }

    /// @description Pull the note's properties from backend.
    function pull_prop() {
        if(noteID == "") return;
        propertyStr = dyc_get_note(noteID);
    	time = propertyStr.time;
    	side = propertyStr.side;
    	width = propertyStr.width;
    	position = propertyStr.position;
    	lastTime = propertyStr.lastTime;
    	noteType = propertyStr.noteType;
        subNoteID = propertyStr.subNoteID;
    	beginTime = propertyStr.beginTime;

        if(noteType == NOTE_TYPE.HOLD && note_is_activated(subNoteID))
            note_get_instance(subNoteID).pull_prop();

        lastAttachBar = -1;
    }
    
    // If a note is moving out of screen, throw a warning.
    _prop_init();
	function note_outscreen_check() {
        var _outbound = false;
        if(side == 0) {
            var _xl = x - pWidth / 2, _xr = x + pWidth / 2;
            if(_xr <= 0 || _xl >= BASE_RES_W)
                _outbound = true;
        }
        else {
            var _yl = y - pWidth / 2, _yr = y + pWidth / 2;
            if(_yr <= 0 || _yl >= BASE_RES_H)
                _outbound = true;
        }
		if(_outbound)
			note_outbound_warning();
	}
	
	// Change to selected state.
	function select() {
		set_state(NOTE_STATES.SELECTED);
		state();
	}

    function cac_LR_side() {
        return x < BASE_RES_W / 2 ? 1:2;
    }

    function change_side(to_side, vis_consistency = (objEditor.editorDefaultWidthMode == 1)) {
        if(vis_consistency) {
            if(side == 0 && to_side > 0)
                width *= 2;
            else if(side > 0 && to_side == 0)
                width /= 2;
        }
        side = to_side;
        if(side == 3)
            side = cac_LR_side();
        _prop_init();
    }

    function on_mixer_side() {
        if(side > 0)
            return objMain.chartSideType[side - 1] == "MIXER";
        return false;
    }

    function update_orig_hold_prop() {
        if(noteType == 2 && note_exists(sinst)) {
            origLength = sinst.time - time;
            origSubTime = sinst.time;
        }
    }

    _prop_init(true);
    
    // _outbound_check was moved to scrNote

// State Machines
    
    /// @param {Enum.NOTE_STATES} toState 
    function set_state(toState) {
        switch toState {
            case NOTE_STATES.OUT:
                state = stateOut;
                break;
            case NOTE_STATES.NORMAL:
                if(stateType == NOTE_STATES.SELECTED)
    		        selectUnlock = true;
                state = stateNormal;
                break;
            case NOTE_STATES.SELECTED:
                state = stateSelected;
                break;
            case NOTE_STATES.ATTACH:
                state = stateAttach;
                break;
            case NOTE_STATES.DROP:
                state = stateDrop;
                break;
            case NOTE_STATES.ATTACH_SUB:
                state = stateAttachSub;
                break;
            case NOTE_STATES.DROP_SUB:
                state = stateDropSub;
                break;
            case NOTE_STATES.LAST:
                state = stateLast;
                break;
            default:
                throw "Unknown note state";
        }
        stateType = toState;
    }

    // State Normal
    function stateNormal() {
        animTargetA = 1.0;
        animTargetLstA = lastAlphaL;
        
        // Side Fading
        if(editor_get_editmode() == 5 && side > 0) {
            animTargetA = lerp(0.25, 1, abs(x - resor_to_x(0.5)) / resor_to_x(0.5-0.2));
            animTargetA = clamp(animTargetA, 0, 1);
        }
        var _limTime = min(objMain.nowTime, objMain.animTargetTime);
        
        // If inbound then the state wont change
        if(!selectInbound) {
            if(time <= _limTime) {
                // If the state in last step is SELECT then skip create_shadow
                if(!selectUnlock)
                    note_hit(get_prop(), true);
                set_state(NOTE_STATES.LAST);
                state();
            }
            if(_outbound_check(x, y, side)) {
                set_state(NOTE_STATES.OUT);
                state();
            }
        }
        
        
        // Check Selecting
        if(editor_get_editmode() == 4 && editor_editside_allowed(side) && !objMain.topBarMousePressed
            && !(objEditor.editorSelectOccupied && noteType == 3)) {
            var _mouse_click_to_select = mouse_isclick_l() && _mouse_inbound_check();
            var _mouse_drag_to_select = mouse_ishold_l() && _mouse_inbound_check(1) 
                && !editor_select_is_area() && !editor_select_is_dragging()
                && !keyboard_check(vk_control);
            
            if(_mouse_click_to_select || _mouse_drag_to_select) {
                objEditor.editorSelectSingleTarget =
                    editor_select_compare(objEditor.editorSelectSingleTarget, id);
            }
            
            if(_mouse_inbound_check()) {
                objEditor.editorSelectSingleTargetInbound = 
                    editor_select_compare(objEditor.editorSelectSingleTargetInbound, id);
            }
        }
    }
    
    // A theme may extend the visible end without moving the chart endpoint.
    function visual_duration() {
        return lastTime;
    }

    // State Last
    function stateLast() {
        animTargetLstA = lastAlphaR;
        
        var _limTime = min(objMain.nowTime, objMain.animTargetTime);
        if(time + visual_duration() <= _limTime) {
            if(!note_exists(sinst) || sinst.stateType != NOTE_STATES.SELECTED) {
                set_state(NOTE_STATES.OUT);
                image_alpha = lastTime == 0 ? 0 : image_alpha;
                state();
            }
        }
        else if(time + lastTime > _limTime && (objMain.nowPlaying || editor_get_editmode() == 5)) {
            partHoldTimer += global.timeManager.get_delta() / 1000;
            partHoldTimer = min(partHoldTimer, 5 * PARTICLE_HOLD_DELAY);
            while(partHoldTimer >= PARTICLE_HOLD_DELAY) {
                partHoldTimer -= PARTICLE_HOLD_DELAY;
                note_emit_particles(PARTICLE_NOTE_LAST, get_prop(), 1);
            }
        }
        
        if(time > objMain.nowTime) {
            set_state(NOTE_STATES.NORMAL);
            state();
        }
    }
    
    // State Targeted
    function stateOut() {
        animTargetA = 0.0;
        animTargetLstA = lastAlphaL;
        
        if(editor_get_editmode() == 5) return;
        if(time + visual_duration() > objMain.nowTime && !_outbound_check(x, y, side)) {
            drawVisible = true;
            if(time + lastTime <= objMain.nowTime) {
                // Scrubbing into the visual-only tail must not replay the note hit.
                animTargetA = 1;
                set_state(NOTE_STATES.LAST);
            }
            else {
                set_state(NOTE_STATES.NORMAL);
            }
	        state();
	    }
    }
    
    // Editors
        // State attached to cursor
        function stateAttach() {
            animTargetA = 0.5;
            
            if(editor_get_note_attaching_center() == id) {
            	if(side == 0) {
	                x = editor_snap_to_grid_x(mouse_x, side);
	                lastAttachBar = editor_snap_to_grid_y(mouse_y, side);
	                y = lastAttachBar.y;
	                position = x_to_note_pos(x, side);
	                time = lastAttachBar.time;
	            }
	            else {
	                y = editor_snap_to_grid_x(mouse_y, side);
	                lastAttachBar = editor_snap_to_grid_y(mouse_x, side);
	                x = lastAttachBar.y;
	                position = x_to_note_pos(y, side);
	                time = lastAttachBar.time;
	            }
	            
	            var _pos = position, _time = time;
	            with(objNote) {
	            	var _center = editor_get_note_attaching_center();
	            	if(stateType == NOTE_STATES.ATTACH) {
	            		position = _pos + origPosition - _center.origPosition;
	            		time = _time + origTime - _center.origTime;
	            		_prop_init();
		            	if(noteType == 2)
		            		_prop_hold_update(false);
	            	}
	            }
            }
            else {
                lastAttachBar = undefined;
            }
            
            if(mouse_check_button_pressed(mb_left) && !_outbound_check(x, y, side)
            	&& id == editor_get_note_attaching_center()) {
                with(objNote) if(stateType == NOTE_STATES.ATTACH) {
                	set_state(NOTE_STATES.DROP);
                	origWidth = width;
                    dropWidthAdjustable = false;
                }
            }
                
        }
        
        // State Dropping down
        function stateDrop() {
            animTargetA = 0.8;
            
            if(mouse_check_button_released(mb_left)) {
                var _singlePaste = objEditor.singlePaste;
                var _toSelectState = _singlePaste;
            	if(editor_get_editmode() > 0)
                	editor_set_default_width(width);
                if(noteType == 2) {
                	if(fixedLastTime != -1) {
                        build_note({
                            noteType: NOTE_TYPE.HOLD,
                            time: time,
                            position: position,
                            width: width,
                            lastTime: fixedLastTime,
                            side: side
                        }, true, _toSelectState, true);
                        if(_singlePaste) instance_destroy();
                        set_state(NOTE_STATES.ATTACH);
                		return;
                	}
                    var _time = time;                    
                    set_state(NOTE_STATES.ATTACH_SUB);
                    sinst = instance_create_depth(x, y, depth, objHoldSub);
                    sinst.dummy = true;
                    sinst.time = time;

                    // Lock the LR side mode.
                    editor_lrside_lock_set(true);
                    return;
                }
                var _newNoteProp = build_note({
                        noteType: noteType,
                        time: time,
                        position: position,
                        width: width,
                        side: side
                    }, true, _toSelectState, true);
                
                // Only do covering check on editmode 1~3.
                if(editor_get_editmode() <= 3 && editor_get_editmode() >= 1) {
                    if(note_check_covered_by_other_note(_newNoteProp))
                        note_cover_warning();
                }
                
                note_outscreen_check();
                
                if(_singlePaste) instance_destroy();
                else {
                    if(editor_get_editmode() == 0)
                        operation_merge_last_request(1, OPERATION_TYPE.PASTE);
                }
                set_state(NOTE_STATES.ATTACH);
                return;
            }
            
            if(!mouse_ishold_l())
            	width = origWidth;
            else {
                if(!dropWidthAdjustable && 
                    abs(side == 0?
                        (2.5 * mouse_get_delta_last_x_l() / 300):
                        (2.5 * mouse_get_delta_last_y_l() / 150)
                        ) > dropWidthError) {
                    mouse_set_last_pos_l();
                    with(objNote) {
                        if(stateType == NOTE_STATES.DROP)
                            dropWidthAdjustable = true;
                    }
                }
                if(dropWidthAdjustable) {
                    if(side == 0)
                        width = origWidth + 2.5 * mouse_get_delta_last_x_l() / 300;
                    else
                        width = origWidth - 2.5 * mouse_get_delta_last_y_l() / 150;
                   
                    width = editor_snap_width(width);
                    width = max(width, 0.01);
                }
            }
            
            _prop_init();
        }
        
        function stateAttachSub() {
            sinst.lastAttachBar = editor_snap_to_grid_y(side == 0?mouse_y:mouse_x, side);
            sinst.time = sinst.lastAttachBar.time;
            
            if(mouse_check_button_pressed(mb_left)) {
                set_state(NOTE_STATES.DROP_SUB);
                origWidth = width;
            }
        }
        
        function stateDropSub() {
            animTargetA = 1.0;
            if(mouse_check_button_released(mb_left)) {
                build_note({
                    noteType: NOTE_TYPE.HOLD,
                    time: time,
                    position: position,
                    width: width,
                    lastTime: sinst.time - time,
                    side: side
                }, true, false, true);
                instance_destroy();
            }
        }
        
        // State Selected
        function stateSelected() {
        	animTargetA = 1;
            animTargetInfoA = 1;
            
            if(editor_get_editmode() != 4)
                set_state(NOTE_STATES.NORMAL);
            
            if(editor_select_is_multiple() && noteType == 3)
                set_state(NOTE_STATES.NORMAL);
            
            // Start dragging selected notes.
            if(!editor_select_is_dragging() && mouse_ishold_l() && _mouse_inbound_check(1)) {
                var _select_self_or_fast_drag = 
                    objEditor.editorSelectedSingleInboundLast == id || objEditor.editorSelectedSingleInboundLast < 0;
                if(!isDragging && _select_self_or_fast_drag && editor_editside_allowed(side)) {
                    isDragging = true;
                    objEditor.editorSelectDragOccupied = 1;
                    if(noteType == 3)
                        editor_lrside_lock_set(true);
                    with(objNote) {
                        if(stateType == NOTE_STATES.SELECTED) {
                        	origProp = get_prop();
                            origX = x;
                            origY = y;
                            origTime = time;
                            origPosition = position;
                            origSide = side;
                            update_orig_hold_prop();
                        }
                    }
                }
            }

            if(isDragging) {
                var _delta_time;
                var _delta_pos;
                var _delta_side;
                /// @self Id.Instance.objNote
                var _calculation = function() {
                    if(side == 0) {
                        lastAttachBar = editor_snap_to_grid_y(origY + mouse_get_delta_last_y_l(), side);
                        y = lastAttachBar.y;
                        x = editor_snap_to_grid_x(origX + mouse_get_delta_last_x_l(), side);
                        position = x_to_note_pos(x, side);
                        time = lastAttachBar.time;
                    }
                    else {
                        lastAttachBar = editor_snap_to_grid_y(origX + mouse_get_delta_last_x_l(), side);
                        x = lastAttachBar.y;
                        y = editor_snap_to_grid_x(origY + mouse_get_delta_last_y_l(), side);
                        position = x_to_note_pos(y, side);
                        time = lastAttachBar.time;
                    }
                }
                _calculation();

                if(objEditor.editorLRSide) {
                    var _side = cac_LR_side();
                    if(_side != side) {
                        side = _side;
                        _calculation();
                    }
                }

                _delta_time = time - origTime;
                _delta_pos = position - origPosition;
                _delta_side = side - origSide;
                
                with(objNote) {
                    if(stateType == NOTE_STATES.SELECTED)
                    {
                        if(objEditor.editorSelectMultiSidesBinding || editor_editside_allowed(side)) {
                            position = origPosition + _delta_pos;
                            time = origTime + _delta_time;
                            if(side > 0)
                                side = (abs((origSide - 1) + _delta_side)&1) + 1;
                        }
                        else {
                            position = origPosition;
                            time = origTime;
                            side = origSide;
                        }
                        if(noteType == 2) {
                            sinst.time = (ctrl_ishold() || editor_select_is_multiple()) ? time + origLength : origSubTime;
                            _prop_hold_update(false);
                        }
                        _prop_init();
                    }
                }
            } else {
                if(!editor_select_is_dragging())
                if(_mouse_inbound_check())
                if(editor_editside_allowed(side))
                    objEditor.editorSelectedSingleInbound =
                        editor_select_compare(objEditor.editorSelectedSingleInbound, id);
            }

            // Stop dragging selected notes.
            if(mouse_check_button_released(mb_left)) {
                if(isDragging) {
                    isDragging = false;
                    
                    if(noteType == 3)
                        editor_lrside_lock_set(false);
                    
                    with(objNote) {
                    	if(stateType == NOTE_STATES.SELECTED) {
                    		operation_step_add(OPERATION_TYPE.MOVE, origProp, get_prop());
                            update_prop();

                            if(noteType == 2)
                                _prop_hold_update();
                    	}
                    	
                    	note_outscreen_check();
                    }
                    
                    note_sort_request();
                }
            }
            
            if((keycheck_down(vk_delete) || keycheck_down(vk_backspace)) && noteType != 3) {
            	note_delete(noteID, true);
            }
                
            
            if(keycheck_down(ord("T")) && editor_select_count() == 1) {
            	timing_point_duplicate(time);
		    }
		    if(keycheck_down_ctrl(vk_delete)) {
		    	timing_point_delete_at(time, true);
		    }
		    if(keycheck_down_ctrl(ord("C")) && !editor_select_is_multiple()) {
		    	editor_set_default_width(width);
		    	announcement_play(i18n_get("copy_width", string_format(width, 1, 2)));
		    }
		    
		    // If double click then send attach request.
		    if(mouse_isclick_double(0) && _mouse_inbound_check(2) && editor_editside_allowed(side)) {
                if(noteType <= 2)
		    	    objEditor.attach(id);
                else
                    objEditor.attach(finst);
		    }
		    
		    // Pos / Time Adjustment
		    var _poschg = (keycheck_down_ctrl(vk_right) - keycheck_down_ctrl(vk_left)) * (shift_ishold() ? 0.05: 0.01);
		    var _timechg = (keycheck_down_ctrl(vk_up) - keycheck_down_ctrl(vk_down)) * (shift_ishold() ? 5: 1);
		    
		    if(_timechg != 0 || _poschg != 0)
                origProp = get_prop();
		    time += _timechg;
		    position += _poschg;
		    if(_timechg != 0) {
		    	note_sort_request();
            }
		    if(_timechg != 0 || _poschg != 0) {
                update_prop();
                operation_step_add(OPERATION_TYPE.MOVE, origProp, get_prop());
            }
        }
        
#macro DYNAMITE_WORLD_PX (BASE_RES_H / 16)
#macro DYNAMITE_TEXEL (DYNAMITE_WORLD_PX / 75)

// ReadNotes sizes the Tap and Slide from the raw chart width.
function dynamite_note_width() {
    var _lane_scale = side == NOTE_SIDE.FRONT ? 40 / 9 : 32 / 15;
    return max(1, width * _lane_scale - 8 / 45) * DYNAMITE_WORLD_PX;
}

/// Draw a horizontal Unity slice about its centre. A negative span mirrors the entire slice.
function dynamite_draw_strip(_spr, _cx, _cy, _span, _left, _right, _tile_middle = false) {
    var _ax = real(side == 0), _ay = (side == 1) - (side == 2);
    var _w = sprite_get_width(_spr), _h = sprite_get_height(_spr);
    var _size = abs(_span), _sign = sign(_span);
    var _border = min(DYNAMITE_TEXEL, _size / (_left + _right));
    var _middle = max(0, _size - (_left + _right) * _border);
    // Tiled borders keep their texel scale. If they cannot fit, use only the start of each source strip.
    var _edge_texel = _tile_middle ? DYNAMITE_TEXEL : _border;
    var _fraction = _tile_middle ? _border / DYNAMITE_TEXEL : 1;
    var _x = _cx - _ax * _span / 2 + _ay * _h * DYNAMITE_TEXEL / 2;
    var _y = _cy - _ay * _span / 2 - _ax * _h * DYNAMITE_TEXEL / 2;
    _ax *= _sign;
    _ay *= _sign;
    dynamite_draw_patch(_spr, 0, 0, _left * _fraction, _h, _x, _y, _sign * _edge_texel, DYNAMITE_TEXEL);
    _x += _ax * _left * _border;
    _y += _ay * _left * _border;

    var _src = _w - _left - _right;
    if (_middle > 0) {
        if (_tile_middle) {
            // Skip offscreen repeats while keeping the texture anchored to the source border.
            var _along = _ax * _x + _ay * _y;
            var _extent = side == 0 ? BASE_RES_W : BASE_RES_H;
            var _limit = max(0, (_ax + _ay) * _extent);
            var _period = _src * DYNAMITE_TEXEL;
            var _first = max(0, floor((_limit - _extent - _along) / _period));
            var _end = min(_middle, _limit - _along);
            for (var _d = _first * _period; _d < _end; _d += _period) {
                var _sw = min(_src, (_middle - _d) / DYNAMITE_TEXEL);
                dynamite_draw_patch(_spr, _left, 0, _sw, _h, _x + _ax * _d, _y + _ay * _d,
                    _sign * DYNAMITE_TEXEL, DYNAMITE_TEXEL);
            }
        } else {
            // Tap has a uniform centre column. The two Hold caps use sliced (stretch) mode.
            dynamite_draw_patch(_spr, _left, 0, _src, _h, _x, _y,
                _sign * _middle / _src, DYNAMITE_TEXEL);
        }
    }
    dynamite_draw_patch(_spr, _w - _right, 0, _right * _fraction, _h, _x + _ax * _middle, _y + _ay * _middle,
        _sign * _edge_texel, DYNAMITE_TEXEL);
}

function dynamite_draw_patch(_spr, _sx, _sy, _sw, _sh, _x, _y, _xs, _ys) {
    draw_sprite_general(_spr, 0, _sx, _sy, _sw, _sh, _x, _y, _xs, _ys, image_angle,
        image_blend, image_blend, image_blend, image_blend, image_alpha);
}

function draw_event() {
    if(!drawVisible) return;
    if(global.style == STYLE.DYNAMITE) {
        dynamite_draw_strip(sprDynamiteTap, x, y, dynamite_note_width(), 30, 30);
    }
    else if(side == 0) {
        draw_sprite_ext(sprNote2, image_number, x - pWidth / 2, y, 
            image_xscale, image_yscale, image_angle, image_blend, image_alpha);
    }
    else {
        draw_sprite_ext(sprNote2, image_number, x, y + pWidth / 2 * (side == 1?-1:1), 
            image_xscale, image_yscale, image_angle, image_blend, image_alpha);
    }
}

function initialize_subnote() {
    if(!note_exists(sinst)) {
        // If the note is dummy, it will not have a subNoteID.
        if(!dyc_note_exists(noteID)) return;
        note_activate(subNoteID, false);
        sinst = note_get_instance(subNoteID);
    }
}

function attach(noteID) {
    global.noteIDMan.update(noteID, id);
    self.noteID = noteID;
    pull_prop();

    if(noteType == NOTE_TYPE.HOLD)
        initialize_subnote();
    
    _prop_init(true);
    if(noteType == NOTE_TYPE.HOLD)
        _prop_hold_update(false);
}

function detach() {
    global.noteIDMan.remove(noteID);
    noteID = "";

    if(noteType == 2)
        sinst.detach();
}

set_state(NOTE_STATES.OUT);
