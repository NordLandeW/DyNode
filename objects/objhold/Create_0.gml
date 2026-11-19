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

		pHeight = max(0, note_lane_speed(side) *
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
    
function visual_duration() {
    var _duration = lastTime;
    if(global.style == STYLE.DYNAMITE) {
        _duration = note_exists(sinst) ? sinst.time - time : lastTime;
        var _lane_speed = note_lane_speed(side);
        if(_lane_speed > 0) {
            // Original 1x travel in px/ms on this lane. DyNode already stores px/ms.
            var _unit_travel = (BASE_RES_H / 16) * 0.025 * (side == NOTE_SIDE.FRONT ? 1 : DYNAMITE_SIDE_SPEED_SCALE);
            _duration = max(_duration, floor(16 * _unit_travel / _lane_speed));
        }
    }
    return _duration;
}

function dynamite_draw_hold(_edges) {
    // The lane coordinate increases from the judgement line toward future notes on all three sides.
    var _ax = real(side == 0), _ay = (side == 1) - (side == 2);
    var _head = _ay * x - _ax * y;
    var _tail = _head + visual_duration() * note_lane_speed(side);
    var _across = _ax * x + _ay * y;
    var _screen_end = side == 1 ? BASE_RES_W : 0;
    var _screen_start = _screen_end - (side == 0 ? BASE_RES_H : BASE_RES_W);
    if (!selectTolerance) {
        _head = max(_head, _screen_start + (side == 0 ? objMain.targetLineBelow : objMain.targetLineBeside));
    }
    if (_tail < _head) return;

    // HoldNScaling sizes the body independently of the Tap/Slide width clamp.
    var _inner_w = max(60, dynamite_chart_span()) - 4 * DYNAMITE_WORLD_PX + 254 * DYNAMITE_TEXEL;
    var _far = min(_tail, _screen_end), _near = max(_head, _screen_start);
    var _visible = _far - _near;
    if (_visible > 0) {
        var _x = _ax * _across + _ay * _far;
        var _y = _ay * _across - _ax * _far;
        if (_edges) {
            var _rail = _inner_w / 2 + 0.02 * DYNAMITE_WORLD_PX;
            var _offset = _rail + sprite_get_width(sprDynamiteHoldSide) * DYNAMITE_TEXEL / 2;
            var _ys = _visible / sprite_get_height(sprDynamiteHoldSide);
            draw_sprite_ext(sprDynamiteHoldSide, 0, _x - _ax * _offset, _y - _ay * _offset,
                DYNAMITE_TEXEL, _ys, image_angle, image_blend, image_alpha);
            draw_sprite_ext(sprDynamiteHoldSide, 0, _x + _ax * _offset, _y + _ay * _offset,
                -DYNAMITE_TEXEL, _ys, image_angle, image_blend, image_alpha);
        } else {
            var _spr = stateType == NOTE_STATES.LAST ? sprDynamiteHoldLit : sprDynamiteHoldInner;
            var _sw = sprite_get_width(_spr), _sh = sprite_get_height(_spr);
            var _ys = 2 * DYNAMITE_TEXEL, _period = _sh * _ys;
            // Inner is flipped vertically about the tail. Clipping must preserve that tile phase.
            var _phase = (_tail - _far) mod _period;
            // Extend the fill beneath the bright rail cores without moving the outline.
            var _fill_w = _inner_w + 6 * DYNAMITE_TEXEL;
            _x -= _ax * _fill_w / 2;
            _y -= _ay * _fill_w / 2;
            for (var _d = 0; _d < _visible; _phase = 0) {
                var _step = min(_period - _phase, _visible - _d);
                _d += _step;
                dynamite_draw_patch(_spr, 0, max(0, _sh - (_phase + _step) / _ys), _sw, _step / _ys,
                    _x - _ay * _d, _y + _ax * _d, _fill_w / _sw, -_ys);
            }
        }
    }
    if (_edges) {
        // A cap can be visible even when its centre is just outside the viewport.
        var _margin = sprite_get_height(sprDynamiteHoldUp) * DYNAMITE_TEXEL / 2;
        if (_tail >= _screen_start - _margin && _tail <= _screen_end + _margin)
            dynamite_draw_strip(sprDynamiteHoldUp, _ax * _across + _ay * _tail, _ay * _across - _ax * _tail,
                _inner_w + 38 * DYNAMITE_TEXEL, 19, 19);
        if (_head >= _screen_start - _margin && _head <= _screen_end + _margin)
            dynamite_draw_strip(sprDynamiteHoldDown, _ax * _across + _ay * _head, _ay * _across - _ax * _head,
                _inner_w + 42 * DYNAMITE_TEXEL, 21, 21);
    }
}

    function draw_dynamix_hold(_draw_edge) {
		
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

function draw_event(_draw_edge) {
    if(!drawVisible) return;
    if(global.style == STYLE.DYNAMITE) {
        dynamite_draw_hold(_draw_edge);
    }
    else if(!subFading || !objMain.nowPlaying) {
        draw_dynamix_hold(_draw_edge);
    }
}

// Correction Value

    dFromBottom = 26;
    uFromTop = 13;
