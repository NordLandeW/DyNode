/// @description 

#macro HOLD_MINIMUM_LENGTH (0.0001)

// Inherit the parent event
event_inherited();

noteType = 2;
holdAlpha = 0.8;
bgLightness = 0.52;

image_yscale = 0.6;

originalHeight = sprite_get_height(sprHoldEdge);
pHeight = originalHeight; // Height in Pixels base on sprHoldEdge

sprite = sprHoldEdge2;

subFading = false;		// Whether the hold is reaching its sub.

_prop_init(true);

// In-Function

	_prop_hold_update = function (sync_to_array = true) {
		if(stateType == NOTE_STATES.OUT) {
			return;
		}
		if(!note_exists(sinst))
			return;

		// Sync the properties
		sinst.position = position;
		sinst.width = width;
		sinst.depth = depth;
		sinst.side = side;
		sinst.finst = id;
		sinst.time = max(sinst.time, time + HOLD_MINIMUM_LENGTH);
		
		if(fixedLastTime != -1)
			sinst.time = time + fixedLastTime;
		
		sinst.beginTime = time;
		if(sync_to_array)
			sinst.update_prop();
		sinst._prop_init(true);

		pHeight = max(0, objMain.playbackSpeed * 
			(sinst.time - max(time, selectTolerance?-9999999:objMain.nowTime)))
			+ dFromBottom + uFromTop;
		subFading = pHeight < originalHeight && stateType == NOTE_STATES.LAST;
		pHeight = max(pHeight, originalHeight);
		lastTime = sinst.time - time;
		lastTime = max(lastTime, 1);
		if(sync_to_array)
			update_prop();
    }
    
    _prop_hold_update();
    
    function draw_event (_draw_edge) {
		if(!drawVisible) return;

		if(subFading && objMain.nowPlaying) return;
		
		// Get Position
		
		gpu_push_state();
		var _nx, _ny;
		    
		if(!selectTolerance) {
		    if(side == 0) {
			    _nx = x;
			    _ny = min(y, BASE_RES_H - objMain.targetLineBelow);
			}
			else {
			    _nx = side == 2? min(x, BASE_RES_W - objMain.targetLineBeside) :
			                    max(x, objMain.targetLineBeside);
			    _ny = y;
			}
		}
		else {
		    _nx = x;
		    _ny = y;
		}
			
		var _h = sprite_get_height(sprHold), _th = pHeight - dFromBottom - uFromTop,
		_w = sprite_get_width(sprHold), _rw = pWidth - _note_get_lrpadding_total(noteType);
		var _sclx = _rw / _w;
		var _scly = pHeight;
		    
		// Optimization
		if(side == 0 && _ny > BASE_RES_H + _h) {
		    var _extra = floor((_ny - BASE_RES_H - _h) / _h) * _h;
		    _ny -= _extra;
		    _th -= _extra;
		    _scly -= _extra;
		}
		else if(side >= 1 && !in_between(_nx, -_h, BASE_RES_W+_h)) {
		    var _extra = floor(max(-_h-_nx, _nx-BASE_RES_W-_h) / _h) * _h;
		    _nx += (side == 1?1:-1) * _extra;
		    _th -= _extra;
		    _scly -= _extra;
		}
		    
		if(side == 0 && _th > BASE_RES_H + 2*_h)
		    _th -= floor((_th - BASE_RES_H - 2*_h) / _h) * _h;
		else if(side >= 1 && _th > BASE_RES_W + 2*_h)
		    _th -= floor((_th - BASE_RES_W - 2*_h) / _h) * _h;
		    
		_scly = min(_scly, (side==0?BASE_RES_H:BASE_RES_W)+3*_h) / originalHeight;
			
		if(!_draw_edge) {
			// Draw Background Sprites
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
		
		// Draw Edge
		else {
			if(side == 0) {
				draw_sprite_ext(sprHoldEdge, image_number,
					_nx - pWidth/2,
					_ny + dFromBottom,
					image_xscale, _scly, image_angle, image_blend, image_alpha);
			}
			else {
				draw_sprite_ext(sprHoldEdge, image_number,
					_nx + dFromBottom * (side == 1? -1: 1),
					_ny + pWidth/2 * (side == 1? -1: 1),
					image_xscale, _scly, image_angle, image_blend, image_alpha);
			}
		}
		gpu_pop_state();
	}

// Correction Value

    dFromBottom = 26;
    uFromTop = 13;