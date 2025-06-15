/// @description Update colored squares

if(editor_get_editmode() <= 4){
	var _col = c_blue;
    
    animTargetInfoA = 0;
    if(editor_select_is_area()) {
        if(editor_select_inbound(x, y, side, noteType)) {
            _col = 0x28caff;
            animTargetNodeA = 1;
        }
        else {
            animTargetNodeA = 0;
        }
    }
    else {
        if((!objEditor.editorSelectOccupied || ctrl_ishold()) && objEditor.editorSelectSingleTargetInbound == id) {
            animTargetNodeA = 1.0;
            animTargetInfoA = ctrl_ishold()? 1:0;
        }
        else if(objEditor.editorHighlightLine) {
        	var _sameside = 
        		objEditor.editorHighlightSide == side ||
        		(objEditor.editorHighlightSide > 0 && side > 0)
            var _position = objEditor.editorHighlightPosition == position
            				&& _sameside;
            var _width = objEditor.editorHighlightWidth == width 
            			 && _sameside;
            var _time = round(objEditor.editorHighlightTime) == round(time);
            if(_position || _width || _time) {
            	animTargetNodeA = 1.0;
            	animTargetInfoA = ctrl_ishold()? 1:0;
            }
            else animTargetNodeA = 0;
            if(_position && _width)
            	_col = 0xa1A1fe;
            else if(_time && _width)
            	_col = 0x50af4c;
            else if(_time)
            	_col = 0x2257ff;
            else if(_position)
            	_col = 0xc2577e;
            else if(_width)
            	_col = 0xFF9E00;
        }
        else animTargetNodeA = 0;
    }
    if(stateType == NOTE_STATES.SELECTED) {
        var _selected_single_inbound = objEditor.editorSelectedSingleInbound == id;
        var _unselection_hint = _selected_single_inbound && objEditor.editorSelectSingleTargetInbound < 0 && ctrl_ishold();
    	if((editor_select_is_area() && editor_select_inbound(x, y, side, noteType)) || _unselection_hint)
            unselectHint = true;
    	else 
            unselectHint = false;
        _col = c_white;
        animTargetNodeA = 1.0;
        animTargetInfoA = 1;

        if(_selected_single_inbound || isDragging || unselectHint)
            animTargetNodeBorderA = 1;
        else
            animTargetNodeBorderA = 0;
    }
    else {
        animTargetNodeBorderA = 0;
    }
	if(stateType == NOTE_STATES.ATTACH || stateType == NOTE_STATES.ATTACH_SUB || stateType == NOTE_STATES.DROP || stateType == NOTE_STATES.DROP_SUB) {
		animTargetNodeA = 0.0;

        if(alt_ishold()) {
            animTargetInfoA = 1;
        }
        else {
            if(id == editor_get_note_attaching_center())
                animTargetInfoA = 1;
            else
                animTargetInfoA = 0.5;
        }
	}
	if((stateType == NOTE_STATES.LAST && noteType == 2) || stateType == NOTE_STATES.OUT) {
		animTargetNodeA = 0;
		animTargetInfoA = 0;
	}

    if(ralt_ishold())
        animTargetInfoA = 1;
	
	if(animTargetNodeA > 0)
	    nodeColor = _col;
}