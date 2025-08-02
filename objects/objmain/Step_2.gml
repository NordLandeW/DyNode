/// @description Mixer Update & Update activated notes array

// Mixer Update

    for(var i=0; i<2; i++) {
        
        // Mixer Restriction
        var _next = mixer_get_next_x(i+1);
        if(!is_undefined(_next)) {
            var _nextX = _next[0];
            var _nextT = _next[1] - objMain.nowTime;
        	mixerNextX[i] = _nextX;
            mixerNextX[i] = clamp(mixerNextX[i], resor_to_y(198/1080), resor_to_y(858/1080));
            
            var _distance = abs(mixerNextX[i] - mixerX[i])
            if(_distance > sprite_get_width(sprMixer) && _distance / min(mixerMaxSpeed, _distance * (1-mixerSpeed)) > _nextT)
                mixerX[i] = mixerNextX[i];
        }
        
        mixerX[i] = lerp_lim_a(mixerX[i], mixerNextX[i], mixerSpeed, mixerMaxSpeed);
        mixerShadow[i].y = mixerX[i];
        mixerShadow[i].x = i*BASE_RES_W + (i? -1:1) * targetLineBeside;
    }

// Get activated notes

    for(var i=0; i<3; i++)
        if(array_length(chartNotesArrayActivated[i]) < 1024)
            array_resize(chartNotesArrayActivated[i], 1024);
    var ap = [0, 0, 0];
    with(objNote) {
        if(noteType <= 2 && !attaching) {
            var _curArray = objMain.chartNotesArrayActivated[noteType];
            _curArray[ap[noteType]] = id;
            ap[noteType]++;
            if(ap[noteType] >= array_length(_curArray)) {
                array_resize(_curArray, array_length(_curArray) * 2);
            }
        }
    }
    for(var i=0; i<3; i++) {
        array_resize(chartNotesArrayActivated[i], ap[i]);
        chartNotesArrayActivated[i] = extern_index_sort(chartNotesArrayActivated[i], function(val) { return -val.priority; });
    }