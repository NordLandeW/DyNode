
pull_prop();

var _editMode = editor_get_editmode();

_prop_init();

if(stateType == NOTE_STATES.OUT && image_alpha<EPS) {
	drawVisible = false;
}
else
    drawVisible = true;

if(_editMode < 5) {
    selectInbound = editor_select_is_area() && editor_select_inbound(x, y, side, noteType, side);
    selectTolerance = selectInbound || stateType == NOTE_STATES.SELECTED;
}
else {
    selectInbound = false;
    selectTolerance = false;
}

state();

if(_editMode < 5)
    update_prop();

selectUnlock = false;

if(drawVisible || nodeAlpha>EPS || infoAlpha>EPS || image_alpha>EPS) {
    if(_editMode < 5) {
        var _factor = 1;
        if(objMain.fadeOtherNotes && !editor_editside_allowed(side))
            _factor = 0.5;
        if(stateType == NOTE_STATES.ATTACH || stateType == NOTE_STATES.SELECTED) {
            if(image_alpha > 0.99)
                image_alpha = animTargetA;      // Prevent weird fade in when attaching notes are reset.
            if(infoAlpha < 0.01)
                infoAlpha = animTargetInfoA;
            if(nodeAlpha < 0.01)
                nodeAlpha = animTargetNodeA;
        }
        image_alpha = lerp_a(image_alpha, animTargetA * _factor,
            animSpeed * (objMain.nowPlaying ? objMain.musicSpeed * animPlaySpeedMul : 1));

        lastAlpha = lerp_a(lastAlpha, animTargetLstA * _factor,
            animSpeed * (objMain.nowPlaying ? objMain.musicSpeed * animPlaySpeedMul : 1));
        
        if(keycheck(ord("A")) || keycheck(ord("D")) || 
            objMain.topBarMousePressed || (side == 0 && objMain.nowPlaying)) {
            image_alpha = animTargetA * _factor;
            lastAlpha = animTargetLstA * _factor;
        }
        
        nodeAlpha = lerp_a(nodeAlpha, animTargetNodeA, animSpeed);
        infoAlpha = lerp_a(infoAlpha, animTargetInfoA, animSpeed);
        nodeBorderAlpha = lerp_a(nodeBorderAlpha, animTargetNodeBorderA, animSpeed);
    }
    else {
        image_alpha = animTargetA;
        lastAlpha = animTargetLstA;
        nodeAlpha = animTargetNodeA;
        infoAlpha = animTargetInfoA;
        nodeBorderAlpha = animTargetNodeBorderA;
    }
}

// If no longer visible then deactivate self
if(!drawVisible && nodeAlpha < EPS && infoAlpha < EPS && !note_is_activated(finst)) {
	note_deactivate(id);
	return;
}

// Update Highlight Line's Position
if(_editMode < 5 && objEditor.editorHighlightLine && note_is_activated(id)) {
	if(stateType == NOTE_STATES.SELECTED && isDragging || stateType == NOTE_STATES.ATTACH_SUB || stateType == NOTE_STATES.DROP_SUB
		|| ((stateType == NOTE_STATES.ATTACH || stateType == NOTE_STATES.DROP) && id == editor_get_note_attaching_center())) {
		objEditor.editorHighlightTime = time;
        objEditor.editorHighlightPosition = position;
        objEditor.editorHighlightSide = side;
        objEditor.editorHighlightWidth = width;
        if(stateType == NOTE_STATES.ATTACH_SUB || stateType == NOTE_STATES.DROP_SUB) {
			objEditor.editorHighlightTime = time + lastTime;
        }
	}
}

// Add selection blend

if(stateType == NOTE_STATES.SELECTED)
    image_blend = selBlendColor;
else
    image_blend = c_white;