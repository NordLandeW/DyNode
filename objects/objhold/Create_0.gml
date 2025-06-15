/// @description 

// Inherit the parent event
event_inherited();

noteType = 2;
holdAlpha = 0.8;
bgLightness = 0.52;

image_yscale = 0.6 * global.scaleYAdjust;

originalHeight = sprite_get_height(sprHoldEdge);
pHeight = originalHeight; // Height in Pixels base on sprHoldEdge

sprite = sprHoldEdge2;

subFading = false;		// Whether the hold is reaching its sub.

_prop_init(true);

// In-Function

	_prop_hold_update = function () {
        if(sinst > 0 || (sid != -1 && ds_map_exists(objMain.chartNotesMap[side], sid))) {
            if(sinst <= 0)
		        sinst = objMain.chartNotesMap[side][? sid]

		    if(stateType != NOTE_STATES.OUT)
		    	note_activate(sinst);
    
		    // Being destroyed
		    if(!instance_exists(sinst))
		        return;
    
		    // Sync the properties
		    sinst.position = position;
		    sinst.width = width;
		    sinst.depth = depth;
		    sinst.side = side;
		    sinst.finst = id;
		    sinst.time = max(sinst.time, time+0.0001);
		    
		    if(fixedLastTime != -1)
		    	sinst.time = time + fixedLastTime;
		    
		    sinst.beginTime = time;
		    sinst.update_prop();
    
    		subFading = false;
		    pHeight = max(0, objMain.playbackSpeed * 
		    	(sinst.time - max(time, selectTolerance?0:objMain.nowTime)))
		        + dFromBottom + uFromTop;
		    if(!global.simplify) {
				subFading = pHeight < originalHeight && stateType == NOTE_STATES.LAST;
		    	pHeight = max(pHeight, originalHeight);
		    }
		    lastTime = sinst.time - time;
		    lastTime = max(lastTime, 1);
			update_prop();
        }
    }
    
    _prop_hold_update();
    
    function draw_event (_draw_edge) {
		if(!drawVisible) return;

		if(subFading && objMain.nowPlaying) return;
		
		// Get Position
		    
		var _nx, _ny;
		    
		if(!selectTolerance) {
		    if(side == 0) {
			    _nx = x;
			    _ny = min(y, global.resolutionH - objMain.targetLineBelow);
			}
			else {
			    _nx = side == 2? min(x, global.resolutionW - objMain.targetLineBeside) :
			                    max(x, objMain.targetLineBeside);
			    _ny = y;
			}
		}
		else {
		    _nx = x;
		    _ny = y;
		}
			
		var _h = sprite_get_height(sprHold), _th = pHeight - dFromBottom - uFromTop,
		_w = sprite_get_width(sprHold), _rw = pWidth - lFromLeft - rFromRight;
		var _sclx = _rw / _w;
		var _scly = pHeight;
		    
		// Optimization
		if(side == 0 && _ny > global.resolutionH + _h) {
		    var _extra = floor((_ny - global.resolutionH - _h) / _h) * _h;
		    _ny -= _extra;
		    _th -= _extra;
		    _scly -= _extra;
		}
		else if(side >= 1 && !in_between(_nx, -_h, global.resolutionW+_h)) {
		    var _extra = floor(max(-_h-_nx, _nx-global.resolutionW-_h) / _h) * _h;
		    _nx += (side == 1?1:-1) * _extra;
		    _th -= _extra;
		    _scly -= _extra;
		}
		    
		if(side == 0 && _th > global.resolutionH + 2*_h)
		    _th -= floor((_th - global.resolutionH - 2*_h) / _h) * _h;
		else if(side >= 1 && _th > global.resolutionW + 2*_h)
		    _th -= floor((_th - global.resolutionW - 2*_h) / _h) * _h;
		    
		_scly = min(_scly, (side==0?global.resolutionH:global.resolutionW)+3*_h) / originalHeight;
			
		if(!_draw_edge) {
		    // Draw Green Blend (Old Workaround)
		    
		    if(global.simplify) {
		    	gpu_set_blendmode(bm_add);
			    draw_set_color_alpha(c_green, lastAlpha * 0.8);
			    if(side == 0)
			    	draw_rectangle(_nx - _rw/2, _ny - _th, _nx + _rw/2, _ny, false);
			    else
			    	draw_rectangle(_nx , _ny - _rw/2, _nx + _th * (side == 1 ? 1 : -1), _ny + _rw / 2, false);
			    draw_set_alpha(1);
		    	gpu_set_blendmode(bm_normal);
		    }
		    
		    // Draw Background Sprites
		    
		    if(!global.simplify) {
		    	if(side == 0) {
			        draw_sprite_part_ext(
			        	global.sprHoldBG[0], 0, 0, 0, _w, _th, _nx - _rw/2, _ny - _th,
			        	_sclx, 1, c_white, holdAlpha * image_alpha);
			        gpu_set_blendmode(bm_add);
				        draw_sprite_ext(
				        	sprHoldGrey, 0, _nx - _rw/2, _ny - _th, 
				        	_sclx, _th / _h, 
				        	0, c_green, lastAlpha * image_alpha * bgLightness);
			        gpu_set_blendmode(bm_normal);
			    }
			    else {
			        draw_sprite_part_ext(
			        	global.sprHoldBG[1], 0, 
			        	0, 0, 
		                _th, _w,
		                _nx + _th * (side == 1 ? 1 : -1), _ny - _rw / 2,
		                side == 2 ? 1 : -1, _sclx, 
		                c_white, holdAlpha * image_alpha);
		            gpu_set_blendmode(bm_add);
				        draw_sprite_ext(
				        	sprHoldGrey, 0,
				        	_nx + _th * (side == 1 ? 1 : -1), _ny - _rw / 2, 
				        	_sclx, (side == 1 ? 1 : -1) * _th / _h,
				        	270, c_green, lastAlpha * image_alpha * bgLightness);
			        gpu_set_blendmode(bm_normal);
			    }
		    }
		}
		
		// Draw Edge
		else {
			var _ica0 = inColorAlp[0] * image_alpha;
			var _ica1 = inColorAlp[1] * image_alpha;
			var _oca0 = outColorAlp[0] * image_alpha;
			var _oca1 = outColorAlp[1] * image_alpha;
			_th = max(_th, 10);
			if(side == 0) {
				if(global.simplify)
					CleanRectangle(_nx - _rw/2, _ny - _th, _nx + _rw/2, _ny)
						.Blend4(inColor[0], _ica0, inColor[0], _ica0, inColor[1], _ica1, inColor[1], _ica1)
						.Border4(7, outColor[0], _oca0, outColor[0], _oca0, outColor[1], _oca1, outColor[1], _oca1)
						.Rounding(2)
						.Draw();
				else
				    draw_sprite_ext(sprHoldEdge, image_number,
				    	_nx - pWidth/2,
				    	_ny + dFromBottom,
				    	image_xscale, _scly, image_angle, image_blend, image_alpha);
			}
			else {
				
				if(global.simplify) {
					var _shape = undefined;
					if(side == 1)
						CleanRectangle(_nx, _ny - _rw/2, _nx + _th, _ny + _rw/2)
							.Blend4(inColor[1], _ica1, inColor[0], _ica0, inColor[1], _ica1, inColor[0], _ica0)
							.Border4(7, outColor[1], _oca1, outColor[0], _oca0,  outColor[1], _oca1, outColor[0], _oca0)
							.Rounding(2)
							.Draw();
					else
						CleanRectangle(_nx - _th, _ny - _rw/2, _nx, _ny + _rw/2)
							.Blend4(inColor[0], _ica0, inColor[1], _ica1, inColor[0], _ica0, inColor[1], _ica1)
							.Border4(7, outColor[0], _oca0, outColor[1], _oca1, outColor[0], _oca0, outColor[1], _oca1)
							.Rounding(2)
							.Draw();
				}
				else
				    draw_sprite_ext(sprHoldEdge, image_number,
				        _nx + dFromBottom * (side == 1? -1: 1),
				        _ny + pWidth/2 * (side == 1? -1: 1), 
				        image_xscale, _scly, image_angle, image_blend, image_alpha);
			}
		}
	}

// Correction Value

    dFromBottom = 26;
    uFromTop = 13; 
    lFromLeft = 12;
    rFromRight = 12;

// Simplification Color Configuration

	inColor = [c_green, c_green];
	inColorAlp = [0.5, 0.7];
	outColor = [scribble_rgb_to_bgr(0xe6ee9c), c_white];
	outColorAlp = [1, 1];