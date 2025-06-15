/// @description Mixer Update & Update activated notes array

// Mixer Update

    for(var i=0; i<2; i++) {
        
        // Mixer Restriction
        var _next = mixer_get_next_x(i+1);
        if(!is_undefined(_next)) {
            var _nextX = _next.x;
            var _nextT = _next.time - objMain.nowTime;
        	mixerNextX[i] = _nextX;
            mixerNextX[i] = clamp(mixerNextX[i], resor_to_y(198/1080), resor_to_y(858/1080));
            
            var _distance = abs(mixerNextX[i] - mixerX[i])
            if(_distance > sprite_get_width(sprMixer) && _distance / min(mixerMaxSpeed, _distance * (1-mixerSpeed)) > _nextT)
                mixerX[i] = mixerNextX[i];
        }
        
        mixerX[i] = lerp_lim_a(mixerX[i], mixerNextX[i], mixerSpeed, mixerMaxSpeed);
        mixerShadow[i].y = mixerX[i];
        mixerShadow[i].x = i*global.resolutionW + (i? -1:1) * targetLineBeside;
    }

// Get activated notes

	array_resize(chartNotesArrayActivated, 1024);
    var ap = 0, lp = 1024;
	with(objNote) {
		if(noteType <= 2 && !attaching) {
            objMain.chartNotesArrayActivated[ap] = arrayPos;
            ap++;
            if(ap >= lp) {
                array_resize(objMain.chartNotesArrayActivated, lp * 2);
                lp *= 2;
            }
        }
    }
    array_resize(chartNotesArrayActivated, ap);
    // Sort by array position
    array_sort(chartNotesArrayActivated, true);