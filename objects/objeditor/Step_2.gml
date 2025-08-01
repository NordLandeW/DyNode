/// @description Update Notes' States

if(editorMode == 4) {
    // If the note being selected selectable
    var _selectable = note_is_activated(editorSelectSingleTarget);
    if(_selectable) {
        with(editorSelectSingleTarget)
            _selectable = _selectable && stateType != NOTE_STATES.SELECTED;
    }

    // If the single note being unselected unselectable1
    var _unselectable = note_is_activated(editorSelectedSingleInbound) && mouse_isclick_l() 
                        && ctrl_ishold() && !_selectable && !mouse_isclick_double(0);
    if(_unselectable) {
        with(editorSelectedSingleInbound)
            _unselectable = _unselectable && stateType == NOTE_STATES.SELECTED && _mouse_inbound_check();
    }

    // Detect if the mouse is dragging to enable selecting area
    if(!note_is_activated(editorSelectSingleTarget) && !editorSelectArea 
        && mouse_ishold_l() && !editorSelectInbound && !editorSelectDragOccupied && !editorSelectSingleTargetInbound) {
            editorSelectArea = true;
            var _pos = mouse_get_last_pos(0);
            editorSelectAreaPosition = xy_to_noteprop(_pos[0], _pos[1], editorSide);
            if(!ctrl_ishold()) {
                editorSelectResetRequest = true;
            }
        }
    if(editorSelectDragOccupied)
        editorSelectArea = false;
    
    // If necessary reset all selected notes
    if(_selectable && !ctrl_ishold()) { 
        editorSelectResetRequest = true;
    }
    if(mouse_isclick_l() && !editorSelectInbound && !_selectable) {
        editorSelectResetRequest = true;
    }
    
    if(editorSelectResetRequest) {
        // For mousewheel width adjust undo
        if(editorWidthAdjustTime < editorWidthAdjustTimeThreshold) {
            editorWidthAdjustTime = editorWidthAdjustTimeThreshold + 1;
            with(objNote) if(stateType == NOTE_STATES.SELECTED) {
                operation_step_add(OPERATION_TYPE.MOVE, origProp, get_prop());
            }
        }
        
        note_select_reset();
        editorSelectResetRequest = false;
    }
    
    // Select a note
    if(_selectable)
        with(editorSelectSingleTarget) {
            set_state(NOTE_STATES.SELECTED);
            state();
        }
    
    // Unselect a note
    if(_unselectable)
        with(editorSelectedSingleInbound) {
            set_state(NOTE_STATES.NORMAL);
            mouse_clear_click();
            state();
        }
    
    // Selecting Area part
    if(editorSelectArea) {
        if(!mouse_ishold_l()) {
            editorSelectArea = false;
            
            with(objNote) {
                if(editor_editside_allowed(side) && 
                    (stateType == NOTE_STATES.NORMAL || stateType == NOTE_STATES.SELECTED) && 
                    editor_select_inbound(x, y, side, noteType)) {
                        set_state(stateType == NOTE_STATES.SELECTED ? NOTE_STATES.NORMAL : NOTE_STATES.SELECTED);
                        state();
                    }
            }
        }
    }
}

// Note's array update & sort
note_sort_all();

#region UNDO & REDO Management

// Operation stack flush
if(array_length(operationStackStep)) {
    operation_step_flush(operationStackStep);
    operationStackStep = [];
}

while(operationCount>MAXIMUM_UNDO_STEPS) {
	array_shift(operationStack);
	operationCount --;
	operationPointer --;
}

#endregion