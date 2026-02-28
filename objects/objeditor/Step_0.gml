/// @description Update Editor

#region Beatlines
    
    var timingPoints = dyc_get_timingpoints();
    animBeatlineTargetAlphaM = editorMode != 5 && array_length(timingPoints);
    beatlineAlphaMul = lerp_a(beatlineAlphaMul, animBeatlineTargetAlphaM, animSpeed);
    if(array_length(timingPoints)) {
        var _modchg = keycheck_down(ord("V")) - keycheck_down(ord("C"));
        var _groupchg = keycheck_down(ord("G"));
        beatlineNowGroup += _groupchg;
        beatlineNowGroup %= 2;
        beatlineNowMode += _modchg;
        beatlineNowMode = clamp(beatlineNowMode, 0, array_length(beatlineModes[beatlineNowGroup])-1);
        
        if(_modchg != 0 || _groupchg != 0) {
        	set_div(beatlineDivs[beatlineNowGroup][beatlineNowMode], false);
            announcement_play(i18n_get("beatline_divs", [string(get_div()),
            	chr(beatlineNowGroup+ord("A"))]), 3000, "beatlineDiv");
        }
        
        if(keycheck_down(192)) {
        	if(editor_set_div())
	        	announcement_play(i18n_get("beatline_divs", [string(get_div()),
	            	chr(beatlineNowGroup+ord("A"))]), 3000, "beatlineDiv");
        }
        
        animBeatlineTargetAlpha[0] += 0.7 * keycheck_down(vk_down);
        animBeatlineTargetAlpha[1] += 0.7 * keycheck_down(vk_left);
        animBeatlineTargetAlpha[2] += 0.7 * keycheck_down(vk_right);

        if(keycheck_down(vk_down) || keycheck_down(vk_left) || keycheck_down(vk_right)) {
            if(editor_get_editmode() == 5)
                editor_set_editmode(4);
        }

        for(var i=0; i<3; i++) {
            if(animBeatlineTargetAlpha[i] > 1.4)
                animBeatlineTargetAlpha[i] = 0;
            beatlineAlpha[i] = lerp_a(beatlineAlpha[i], min(animBeatlineTargetAlpha[i], 1), animSpeed);
        }
        
        for(var i=0; i<=beatlineMaxDiv; i++)
            beatlineEnabled[i] = 0;
        if(beatlineDivs[beatlineNowGroup][beatlineNowMode] == get_div()) {
        	var l = array_length(beatlineModes[beatlineNowGroup][beatlineNowMode]);
	        for(var i=0; i<l; i++)
	            beatlineEnabled[beatlineModes[beatlineNowGroup][beatlineNowMode][i]] = 1;
        }
        else {
        	beatlineEnabled[1] = 1;
            if(get_div()<=beatlineMaxDiv)
        		beatlineEnabled[get_div()] = 1;
        }
    }
    else {
        if(keycheck_down(vk_down) || keycheck_down(vk_left) || keycheck_down(vk_right)) {
            announcement_warning("beatline_without_timing");
        }
    }

#endregion

#region Note Edit

    // Wheel width adjust
    var _delta_width = wheelcheck_up_ctrl() - wheelcheck_down_ctrl();
    if(_delta_width != 0) {
        with(objNote) if(stateType == NOTE_STATES.SELECTED) {
            if(!is_struct(origPropWidthAdjust))
                origPropWidthAdjust = get_prop();
            width += _delta_width * 0.05;
            update_prop();
            objEditor.editorWidthAdjustTime = 0;
        }
    }
    
    if(editorWidthAdjustTime < editorWidthAdjustTimeThreshold) {
        editorWidthAdjustTime += global.timeManager.get_delta() / 1000;
        if(editorWidthAdjustTime >= editorWidthAdjustTimeThreshold) {
            with(objNote) if(stateType == NOTE_STATES.SELECTED) {
                operation_step_add(OPERATION_TYPE.MOVE, origPropWidthAdjust, get_prop());
                origPropWidthAdjust = -1;
            }
            operation_merge_last_request(1, OPERATION_TYPE.SETWIDTH);
        }
    }
    editorWidthAdjustTime = min(editorWidthAdjustTime, 10000);

#endregion