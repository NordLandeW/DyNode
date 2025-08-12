function generate_lazer_sprite(_height) {
	var _spr = -1;
	var _surf = surface_create(1, _height);

	gpu_set_blendmode_ext(bm_one, bm_zero);
	shader_set(shd_lazer);
	shader_set_uniform_q("mean", 1.0);
	shader_set_uniform_q("stdDev", 0.25);
		surface_set_target(_surf);
			draw_surface(_surf, 0, 0);
		surface_reset_target();
	shader_reset();
	gpu_set_blendmode(bm_normal);
	
	_spr = sprite_create_from_surface(_surf, 0, 0, 1, _height, 0, 0, 0, _height);
	
	surface_free(_surf);
	
	return _spr;
}

function array_fill(arr, val, index, num) {
	for(var i=index; i<index+num; i++)
		arr[i] = val;
}

#region HOLD DRAW

function generate_hold_sprite(_height) {
	var _ret = [];
	
	
	var _w = sprite_get_width(sprHold);
	var _h = sprite_get_height(sprHold);
	
	gpu_set_blendmode_ext(bm_one, bm_zero);
	// Vertical Sprite
	var _surf = surface_create(_w, _height);
	surface_set_target(_surf);
		draw_sprite_stretched(sprHold, 0, 0, 0, _w, _height);
	surface_reset_target();
	_ret[0] = sprite_create_from_surface(_surf, 0, 0, _w, _height, false, false, 0, 0);
	surface_free_f(_surf);
	
	// Horizontal Sprite
	_surf = surface_create(_height, _w);
	surface_set_target(_surf);
		draw_sprite_ext(_ret[0], 0, 0, _w, 1, 1, 90, c_white, 1);
	surface_reset_target();
	_ret[1] = sprite_create_from_surface(_surf, 0, 0, _height, _w, false, false, 0, 0);
	surface_free_f(_surf);
	gpu_set_blendmode(bm_normal);
	
	return _ret;
}

#endregion

#region DRAW
function draw_sprite_stretched_exxt(sprite, subimg, x, y, w, h, rot, col, alpha) {
	var _xscl = w / sprite_get_width(sprite);
	var _yscl = h / sprite_get_height(sprite);
	draw_sprite_ext(sprite, subimg, x, y, _xscl, _yscl, rot, col, alpha);
}

function draw_set_color_alpha(color, alpha) {
	draw_set_color(color);
	draw_set_alpha(alpha);
}

function draw_scribble_box(_ele, x, y, alpha) {
	var _bbox = _ele.get_bbox(x, y);
	
	CleanRectangle(_bbox.left-5, _bbox.top-5, _bbox.right+5, _bbox.bottom+5)
		.Blend(merge_color(theme_get().color, c_black, 0.4), alpha)
		.Rounding(10)
		.Draw();
}

function draw_rectangle_gradient(position, color, alpha) {
	draw_primitive_begin(pr_trianglelist);
	
		draw_vertex_color(position[0], position[1], color[0], alpha[0]);
		draw_vertex_color(position[2], position[3], color[3], alpha[3]);
		draw_vertex_color(position[0], position[3], color[2], alpha[2]);
		
		draw_vertex_color(position[0], position[1], color[0], alpha[0]);
		draw_vertex_color(position[2], position[1], color[1], alpha[1]);
		draw_vertex_color(position[2], position[3], color[3], alpha[3]);
	
	draw_primitive_end();
}

// W Pixels Width
function generate_pause_shadow(height, indent = 30) {
	var width = BASE_RES_W;
	var surf = surface_create(width, height+2*indent);
	
	gpu_set_blendmode_ext(bm_one, bm_zero);
	shader_set(shd_lazer);
	shader_set_uniform_q("mean", 0.5);
	shader_set_uniform_q("stdDev", 0.15);
		surface_set_target(surf);
			draw_surface(surf, 0, 0);
		surface_reset_target();
	shader_reset();
	gpu_set_blendmode(bm_normal);
	
	var _spr = sprite_create_from_surface(surf, 0, 0, width, height+2*indent, false, false, 0, 0);
	surface_free(surf);
	
	return _spr;
}
#endregion

#region TIME & BAR & BPM

function time_to_bar(time, barpm = objMain.chartBarPerMin) {
    return time * barpm / 60000;
}

function bar_to_time(offset, barpm = objMain.chartBarPerMin) {
    return offset * 60000 / barpm;
}

function mtime_to_time(mtime, offset = objMain.chartTimeOffset) {
	return mtime + offset;
}
function time_to_mtime(time, offset = objMain.chartTimeOffset) {
	return time - offset;
}

function bpm_to_mspb(bpm) {
	return 60 * 1000 / bpm;
}
function mspb_to_bpm(mspb) {
	return 60 * 1000 / mspb;
}

// Especially for dym
function time_to_bar_for_dym(time) {
	var timingPoints = dyc_get_timingpoints();
	var _rbar = 0;
	var l = array_length(timingPoints);
	for(var i=0; i<l; i++) {
		if(i>0)
			_rbar += time_to_bar(timingPoints[i].time - timingPoints[i-1].time,
				mspb_to_bpm(timingPoints[i-1].beatLength)/4);
		if(time < timingPoints[0].time || 
			i == l-1 ||
			in_between(time, timingPoints[i].time, timingPoints[i+1].time)) 
			{
				return _rbar +
					time_to_bar(time - timingPoints[i].time,
						mspb_to_bpm(timingPoints[i].beatLength)/4);
			}
	}
	
	show_error("CONVERSION FATAL ERROR", true);
}

// Accurate bar in DyNode's concept.
// limit argument only used by timing_fix.
function time_to_bar_dyn(time, limit = 0x7fffffff) {
	var timingPoints = dyc_get_timingpoints();
	if (!array_length(timingPoints)) return 0;

	var totalBars = 1;
	var nowAt = 0;
	var l = array_length(timingPoints);

	while (nowAt + 1 != l && timingPoints[nowAt + 1].time <= time+1) {	// Error correction.
		if(timingPoints[nowAt + 1].time > limit) break;
		totalBars += ceil((timingPoints[nowAt + 1].time - timingPoints[nowAt].time) /
			(timingPoints[nowAt].beatLength * timingPoints[nowAt].meter));
		nowAt++;
	}

	var nowTP = timingPoints[nowAt];
	var nowBeats = (time - nowTP.time) / nowTP.beatLength;
	var nowBars = nowBeats / nowTP.meter;

	return totalBars + nowBars;
}

function bar_to_time_dyn(bar) {
	var timingPoints = dyc_get_timingpoints();
	if (!array_length(timingPoints) || bar <= 0) return 0;

	var totalBars = 1;
	var nowAt = 0;
	var l = array_length(timingPoints);

	while (nowAt + 1 != l) {
		var nextTotalBars = totalBars + ceil((timingPoints[nowAt + 1].time - timingPoints[nowAt].time) /
			(timingPoints[nowAt].beatLength * timingPoints[nowAt].meter));

		if (nextTotalBars > bar) break;
		totalBars = nextTotalBars;
		nowAt++;
	}

	var nowTP = timingPoints[nowAt];
	var remainingBars = bar - totalBars;
	var remainingBeats = remainingBars * nowTP.meter;
	var time = nowTP.time + remainingBeats * nowTP.beatLength;

	return time;
}

#endregion

#region POSITION TRANSFORM
function note_time_to_pix(_time) {
	return _time * objMain.playbackSpeed;
}
function pix_to_note_time(_pixel) {
	return _pixel / objMain.playbackSpeed;
}

function note_pos_to_x(_pos, _side) {
    if(_side == 0) {
        return BASE_RES_W/2 + (_pos-2.5)*300;
    }
    else {
        return BASE_RES_H/2 + (2.5-_pos)*150;
    }
}
function x_to_note_pos(_x, _side) {
	if(_side == 0) {
		return (_x - BASE_RES_W / 2) / (300) + 2.5;
	}
	else {
		return 2.5 - (_x - BASE_RES_H / 2) / 150;
	}
}
function y_to_note_time(_y, _side) {
	if(_side == 0) {
		return (BASE_RES_H - objMain.targetLineBelow - _y) / objMain.playbackSpeed + objMain.nowTime;
	}
	else {
		_y = _side == 1? _y: BASE_RES_W - _y;
		return (_y - objMain.targetLineBeside) / objMain.playbackSpeed + objMain.nowTime;
	}
}
function note_time_to_y(_time, _side) {
	if(_side == 0) {
		return BASE_RES_H - objMain.targetLineBelow - (_time - objMain.nowTime) * objMain.playbackSpeed;
	}
	else {
		return BASE_RES_W / 2 + (_side == 1?-1:1) *  (BASE_RES_W / 2 - 
			(objMain.playbackSpeed * (_time - objMain.nowTime)) - objMain.targetLineBeside);
	}
}
function noteprop_to_xy(_pos, _time, _side) {
	if(_side == 0)
		return [note_pos_to_x(_pos, _side), note_time_to_y(_time, _side)];
	else
		return [note_time_to_y(_time, _side), note_pos_to_x(_pos, _side)];
}
// Struct is slow.
function noteprop_set_xy(_pos, _time, _side) {
	if(_side == 0) {
		x = note_pos_to_x(_pos, _side);
		y = note_time_to_y(_time, _side);
	}
	else {
		x = note_time_to_y(_time, _side);
		y = note_pos_to_x(_pos, _side);
	}
}
function xy_to_noteprop(_x, _y, _side) {
	if(_side == 0) {
		return {
			pos: x_to_note_pos(_x, _side),
			time: y_to_note_time(_y, _side)
		};
	}
	else
		return {
			pos: x_to_note_pos(_y, _side),
			time: y_to_note_time(_x, _side)
		};
}
function resor_to_x(ratio) {
    return BASE_RES_W * ratio;
}
function resor_to_y(ratio) {
    return BASE_RES_H * ratio;
}
#endregion

#region LOAD & SAVE CHART
function difficulty_num_to_char(_number) {
	return string_char_at(global.difficultyString, _number + 1);
}
function difficulty_char_to_num(_char) {
    var _pos = string_last_pos(_char, global.difficultyString);
    if(_pos == 0) {
        show_debug_message($"Warning: Undefined difficulty string - {_char}");
        _pos = string_length(global.difficultyString);
    }
	return _pos - 1;
}
function difficulty_num_to_name(_number) {
	return global.difficultyName[_number];
}

function note_type_num_to_string(_number) {
	return global.noteTypeName[_number];
}

#endregion

function has_cjk(str) {
	var i=1, l=string_length(str);
	for(; i<=l; i++) {
		if(ord(string_char_at(str, i)) >= 255)
			return true;
	}
	return false;
}

function string_real(str) {
	var _dot = false, _ch, _ret = "";
	for(var i=1, l=string_length(str); i<=l; i++) {
		_ch = string_char_at(str, i);
		_ch = ord(_ch);
		if(_ch >= ord("0") && _ch <=ord("9"))
			_ret += chr(_ch);
		else if(!_dot && _ch == ord(".")) {
			_dot = true;
			_ret += chr(_ch);
		}
	}
	return _ret;
}

function cjk_prefix() {
	return "[sprMsdfNotoSans]";
}


#region FORMAT FUNCTIONS
function format_markdown(_str) {
	_str = string_replace_all(_str, "**", "");
	_str = string_replace_all(_str, "* ", "· ")
	return _str;
}

function format_time_ms(_time) {
	return string_format(_time, 1, 1) + "ms";
}

// Convert time (in ms) to standard string format
function format_time_string(_time) {
	var _min = floor(_time / 1000 / 60);
	var _sec = floor((_time - _min * 60000)/1000);
	var _ms = floor(_time - _min * 60000 - _sec * 1000);
	var _str = string_format(_min, 2, 0) + ":" + string_format(_sec, 2, 0) + ":" + string_format(_ms, 3, 0);
	_str = string_replace_all(_str, " ", "0");
	return _str;
}

// Convert time (in ms) to standard string format in hh:mm:ss
function format_time_string_hhmmss(_time) {
    var _hour = floor(_time / 1000 / 60 / 60);
    var _min = floor((_time - _hour * 3600000) / 1000 / 60);
    var _sec = floor((_time - _hour * 3600000 - _min * 60000) / 1000);
    var _str = string_format(_hour, 2, 0) + ":" + string_format(_min, 2, 0) + ":" + string_format(_sec, 2, 0);
    _str = string_replace_all(_str, " ", "0");
    return _str;
}

#endregion

#region FAST IO

/// @description Warning: This function wipes out all [dst] data.
function fast_buffer_copy(dst, src, size) {
	DyCore_buffer_copy(buffer_get_address(dst), buffer_get_address(src), size);

	buffer_set_used_size(dst, size);
}

function fast_file_save_buffer_async(file, buffer) {
	file = file_path_fix(file);
	var _len = buffer_get_size(buffer);
	var _buf = buffer_create(_len, buffer_fixed, 1);
	fast_buffer_copy(_buf, buffer, _len);
	var _id = buffer_save_async(_buf, file, 0, _len);
	return {
		id: _id,
		buffer: _buf
	};
}

function fast_file_save_buffer(file, buffer) {
	file = file_path_fix(file);
	buffer_seek(buffer, buffer_seek_start, 0);
	print_buffer_hex(buffer);
	buffer_save(buffer, file);
	return;
}

function fast_file_save_async(file, str) {
	file = file_path_fix(file);
	var _len = string_byte_length(str);
	var _buf = buffer_create(_len, buffer_fixed, 1);
	buffer_seek(_buf, buffer_seek_start, 0);
	buffer_write(_buf, buffer_text, str);
	var _id = buffer_save_async(_buf, file, 0, _len);
	return {
		id: _id,
		buffer: _buf
	};
}

function fast_file_save(file, str) {
	file = file_path_fix(file);
	var _len = string_byte_length(str);
	var _buf = buffer_create(_len, buffer_fixed, 1);
	buffer_seek(_buf, buffer_seek_start, 0);
	buffer_write(_buf, buffer_text, str);
	buffer_save(_buf, file);
	buffer_delete(_buf);
	return;
}

function file_path_fix(_file) {
	if(os_type == os_windows && string_last_pos(SYSFIX, _file) == 0)
		_file = SYSFIX + _file;
	return _file;
}
#endregion

#region WINDOWS

function window_reset() {
	var _ratio = min(display_get_width() / BASE_RES_W, display_get_height() / BASE_RES_H) * objManager.windowDisplayRatio;
	var w = BASE_RES_W * _ratio, h = BASE_RES_H * _ratio
    window_set_rectangle(
		(display_get_width() - w) * 0.5,
		(display_get_height() - h) * 0.5,
		w,
		h
	);
}

#endregion

function in_between(x, l, r) {
	var _l, _r;
	if(l>r) {
		_l = r; _r = l;
	}
	else {
		_l = l; _r = r;
	}

	return x >= _l && x <= _r;
}
function pos_inbound(xo, yo, x1, y1, x2, y2, onlytime = -1) {
	if(onlytime == 0)
		return in_between(yo, y1, y2);
	else if(onlytime > 0)
		return in_between(xo, x1, x2);
	return in_between(xo, x1, x2) && in_between(yo, y1, y2);
}

function array_top(array) {
    return array[array_length(array)-1];
}

function lerp_lim(from, to, amount, limit) {
    var _delta = lerp_safe(from, to, amount)-from;
    
    _delta = min(abs(_delta), limit) * sign(_delta);
    
    return from+_delta;
}

// Fix: Keep the lerp animation frame-rate independent
// (based on the parameters tweaked on 165FPS, which is now using a fix parameter instead
// to keep them work normally)
function lerp_lim_a(from, to, amount, limit) {
	var fix_parameter = 60 / 165;
    return lerp_lim(from, to, 
        1 - power(1 - amount * fix_parameter, delta_time / 1000000 * 165),
		limit * global.fpsAdjust);
}

function lerp_a(from, to, amount) {
	if(from == to) return from;
	var fix_parameter = 60 / 165;
	return lerp_safe(from, to, 1 - power(1 - amount * fix_parameter, delta_time / 1000000 * 165));
}

function lerp_safe(from, to, amount) {
	if(abs(to-from)<LERP_EPS) return to;
	return lerp(from, to, amount);
}

function lerp_pos(from, to, amount) {
	return {
		x: lerp_safe(from.x, to.x, amount),
		y: lerp_safe(from.y, to.y, amount)
	};
}

function lerp_a_pos(from, to, amount) {
	return lerp_pos(from, to, amount * global.fpsAdjust);
}

function create_scoreboard(_x, _y, _dep, _dig, _align, _lim) {
	/// @type {Id.Instance.objScoreBoard} 
    var _inst = instance_create_depth(_x, _y, _dep, objScoreBoard);
    _inst.align = _align;
    _inst.preZero = _dig;
    _inst.scoreLimit = _lim;
    return _inst;
}

function random_id(_length) {
	var chrset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	var _ret = "", _l = string_length(chrset);
	repeat(_length) {
		_ret += string_char_at(chrset, irandom_range(1, _l));
	}
	return _ret;
}

// Compress sprite using better scaling
function compress_sprite(_spr, _scale, _center = false){
	var _w = sprite_get_width(_spr);
	var _h = sprite_get_height(_spr);
	_w = ceil(_w*_scale);
	_h = ceil(_h*_scale);
	
	var _surf = surface_create(_w, _h);
	draw_clear_alpha(c_black, 0);
	surface_set_target(_surf);
    	better_scaling_draw_sprite(_spr, 0, 0, 0, _scale, _scale, 0, c_white, 1, 1);
    	
    	var _xorig = _center ? _w / 2 : 0;
    	var _yorig = _center ? _h / 2 : 0;
    	var _rspr = sprite_create_from_surface(_surf, 0, 0, _w, _h, 0, 0,
    	    _xorig, _yorig);
	surface_reset_target();
	surface_free(_surf);
	
	return _rspr; 
}

// Get a blured application surface
function get_blur_appsurf() {
	var _w = surface_get_width(application_surface);
	var _h = surface_get_height(application_surface);
	var u_size = shader_get_uniform(shd_gaussian_blur_2pass, "size");
    var u_blur_vector = shader_get_uniform(shd_gaussian_blur_2pass, "blur_vector");
	
	var _ret = surface_create(_w, _h);
	var _pong = surface_create(_w, _h);
	
	surface_set_target(_pong);
		shader_set(shd_gaussian_blur_2pass);
			shader_set_uniform_f_array(u_size, [_w, _h, 20, 10]);
			shader_set_uniform_f_array(u_blur_vector, [0, 1]);
			draw_surface(application_surface, 0, 0);
		shader_reset();
	surface_reset_target();
	
	surface_set_target(_ret);
		shader_set(shd_gaussian_blur_2pass);
			shader_set_uniform_f_array(u_size, [_w, _h, 20, 10]);
			shader_set_uniform_f_array(u_blur_vector, [1, 0]);
			draw_surface(_pong, 0, 0);
		shader_reset();
	surface_reset_target();
	
	surface_free_f(_pong);
	return _ret;
}

// Draw some shapes on blurred application surface and return
function get_blur_shapesurf(func) {
	if(!is_method(func))
		show_error("func must be a method.", true);
	
	var _surf = get_blur_appsurf();
	var _w = surface_get_width(application_surface);
	var _h = surface_get_height(application_surface);
	var _temp = surface_create(_w, _h)
	surface_set_target(_temp);
		draw_clear_alpha(c_black, 0);
		func();
	surface_reset_target();
	
	surface_set_target(_surf);
		gpu_set_blendmode_ext(bm_zero, bm_src_alpha);
		draw_surface(_temp, 0, 0);
		gpu_set_blendmode(bm_normal);
	surface_reset_target();
	
	surface_free_f(_temp);
	
	return _surf;
}

function surface_checkate(_surf, _w, _h) {
	if(!surface_exists(_surf))
		return surface_create(_w, _h);
	return _surf;
}

function surface_free_f(_surf) {
    if(surface_exists(_surf))
        surface_free(_surf);
}

function io_clear_diag() {
	keyboard_clear(keyboard_lastkey);
	mouse_clear(mouse_lastbutton);
	io_clear();
}

function show_question_i18n(str) {
	return show_question(i18n_get(str));
}

function show_error_i18n(str, abort) {
	return show_error(i18n_get(str), abort);
}

function get_string_i18n(str, def) {
	return get_string(i18n_get(str), def);
}

function show_debug_message_safe(str) {
	if(DEBUG_MODE)
		show_debug_message(str);
}

function version_cmp(vera, verb) {
	var _version_deal = function (ver) {
		ver = string_replace(ver, "-dev", ".-1");
		ver = string_replace(ver, "v", "");
		return string_split(ver, ".");
	}
	var arra = _version_deal(vera);
	var arrb = _version_deal(verb);
	var la = array_length(arra), lb = array_length(arrb);
	try {
		for(var i=0; i<la; i++)
			arra[i] = int64(arra[i]);
		for(var i=0; i<lb; i++)
			arrb[i] = int64(arrb[i]);
	} catch (e) {
		announcement_error("Weird version number found in config/beatmap file.\n"+vera+"\n"+verb);
		return 0;
	}
	
	for(var i=0; i<la || i<lb; i++) {
		if(i>=la && i<lb)
			arra[i] = 0;
		if(i<la && i>=lb)
			arrb[i] = 0;
		if(arra[i]<arrb[i])
			return -1;
		if(arra[i]>arrb[i])
			return 1;
	}
	if(la!=lb) return la<lb?-1:1;
	return 0;
}

function color_invert(col) {
	var _r = 255-color_get_red(col);
	var _g = 255-color_get_green(col);
	var _b = 255-color_get_blue(col);
	return make_color_rgb(_r, _g, _b);
}

function assert(expression) {
	if(!expression)
		throw "Assertion Failed.";
}

function filename_name_no_ext(file_name) {
	return string_replace(filename_name(file_name), filename_ext(file_name), "");
}

function array_lower_bound(array, lim, fetch_function=undefined) {
	if(fetch_function == undefined)
		fetch_function = function(array, at) { return array[at]; };

	var l = 0, r = array_length(array), mid;
	while(l!=r) {
		mid = (l+r)>>1;
		if(fetch_function(array, mid) < lim) l = mid+1;
		else r = mid;
	}
	return l;
}

function array_upper_bound(array, lim, fetch_function=undefined) {
	if(fetch_function == undefined)
		fetch_function = function(array, at) { return array[at]; };

	var l = 0, r = array_length(array), mid;
	while(l!=r) {
		mid = (l+r)>>1;
		if(fetch_function(array, mid) <= lim) l = mid+1;
		else r = mid;
	}
	return l;
}


/**
 * @description Converts a MIME+Base64 encoded string to a file in the temporary directory.
 *              This function supports different media categories including audio, video, and image.
 * @param {String} category - The media category to process. Valid options are "audio", "video", "image".
 * @param {String} base64_string - The Base64 encoded string with MIME type prefix.
 * @param {String} [file_prefix=""] - Optional prefix for the output file name. Defaults to an empty string.
 * @returns {String} The path to the converted file in the temporary directory. Returns an empty string
 *                   if the MIME type is unsupported or if the Base64 string is invalid.
 *
 * @example
 * Example usage
 * var file_path = convert_mime_base64_to_file("audio", "data:audio/mpeg;base64,SSBsb3ZlIHNjaWVuY2U=");
 * console.log("File saved to: " + file_path);
 *
 * @note The function will show a debug message and return an empty string if the MIME type is not supported.
 *       It also assumes that the MIME type and Base64 data are correctly formatted in the input string.
 */
function convert_mime_base64_to_file(category, base64_string, file_prefix = "") {
	if(string_length(base64_string) < 12) return "";
	if(file_prefix != "")
		file_prefix += "_";
	static mime_map = {
		audio: {
			"audio/mpeg": ".mp3",
			"audio/wav": ".wav",
			"audio/aac": ".aac",
			"audio/ogg": ".ogg",
			"audio/midi": ".midi",
			"audio/flac": ".flac"
		},
		video: {
			"video/mp4": ".mp4",
			"video/x-msvideo": ".avi",
			"video/quicktime": ".mov",
			"video/x-ms-wmv": ".wmv",
			"video/x-flv": ".flv",
			"video/x-matroska": ".mkv"
		},
		image: {
			"image/jpeg": ".jpeg",
			"image/png": ".png",
			"image/gif": ".gif",
			"image/bmp": ".bmp",
			"image/svg+xml": ".svg",
			"image/tiff": ".tiff"
		}
	};

	var _current_map = struct_get(mime_map, category);
    var _start = string_pos(";", base64_string) + 1;
    var _end = string_pos(",", base64_string);
    var mime_type = string_copy(base64_string, 6, _start - 6 - 1);
    var base64_data = string_copy(base64_string, _end + 1, string_length(base64_string) - _end);

    var buffer = buffer_base64_decode(base64_data);

    var file_extension = struct_get(_current_map, mime_type);
    
    if (file_extension != undefined) {
        var file_name = temp_directory + file_prefix + category + file_extension;
        buffer_save(buffer, file_name);
		buffer_delete(buffer);
		return file_name;
    } else {
		show_debug_message("Unsupported mime type.");
		buffer_delete(buffer);
		return "";
    }
}

function surface_clear(surface) {
	var orig_target = surface_get_target();
	if(orig_target > 0) surface_reset_target();

	surface_set_target(surface);
		draw_clear_alpha(c_black, 0);
	surface_reset_target();

	if(orig_target > 0) surface_set_target(surface);
}

function print_buffer_hex(buffer) {
    var output = "";
    var byte;
    
    var length = buffer_get_size(buffer);
    var bytes_to_read = min(length, 100);
    
    for (var i = 0; i < bytes_to_read; i++) {
        byte = buffer_peek(buffer, i, buffer_u8);
        output += string(int64(byte));
        if ((i + 1) % 16 == 0) {
            output += "\n";
        } else {
            output += " "; 
        }
    }
    
    if (bytes_to_read % 16 != 0) {
        output += "\n";
    }
    
    show_debug_message(output);
}

/// @description Convert rgb color to normalized hsv color arrays.
/// @param {Real} rgb Color
/// @returns {Array<Real>} HSV Array [H, S, V]
function color_rgb_to_hsv(rgb) {
    var r = color_get_red(rgb);
    var g = color_get_green(rgb);
    var b = color_get_blue(rgb);
    
    r /= 255;
    g /= 255;
    b /= 255;
    
    var max_val = max(r, max(g, b));
    var min_val = min(r, min(g, b));
    var delta = max_val - min_val;
    
    var h = 0;
    var s = (max_val == 0) ? 0 : delta / max_val;
    var v = max_val;
    
    if (delta != 0) {
        if (max_val == r) {
            h = ((g - b) / delta) % 6;
        } else if (max_val == g) {
            h = (b - r) / delta + 2;
        } else { // max_val == b
            h = (r - g) / delta + 4;
        }
        
        h *= 60;
        if (h < 0) h += 360;
    }
    
    return [h/360, s, v];
}



/// @description Quickly set the shader's uniform with reals/ints or their arrays.
/// @param {String} uniform Uniform's name.
/// @param {Real|Array<Real>} vals Values to set.
/// @param {Bool} real Is values real number or integers.
/// @param {Asset.GMShader} shd The shader.
function shader_set_uniform_q(uniform, vals, real = true, shd = undefined) {
    if(shd == undefined) shd = shader_current();
    var _uniform = shader_get_uniform(shd, uniform);
    if(is_array(vals)) {
        if(real)
            shader_set_uniform_f_array(_uniform, vals);
        else
            shader_set_uniform_i_array(_uniform, vals);
    }
    else if(real)
        shader_set_uniform_f(_uniform, vals);
    else
        shader_set_uniform_i(_uniform, vals);
}

/// @description Quickly set the shader's samplers.
/// @param {String} uniform Uniform's name.
/// @param {Pointer.Texture} texture Textures to attach.
/// @param {Asset.GMShader} shd The shader.
function shader_set_texture(uniform, texture, shd = undefined) {
    if(shd == undefined) shd = shader_current();
    var _sampler = shader_get_sampler_index(shd, uniform);
    texture_set_stage(_sampler, texture);
}

/// @description Get the safe delta time limited under 10FPS.
function get_delta_time() {
	return min(delta_time, 100000);
}

/// @description Delete an element from an array at a specific index without preserving order.
/// @param {Array} arr The array to modify.
/// @param {Real} index The index of the element to delete.
function array_delete_fast(arr, index) {
	if (index < 0 || index >= array_length(arr)) return;

	arr[index] = arr[array_length(arr) - 1];
	array_pop(arr);
}

/// @description Map a value from one range to another.
/// @param {Real} value The value to map.
/// @param {Real} in_min The minimum of the input range.
/// @param {Real} in_max The maximum of the input range.
/// @param {Real} out_min The minimum of the output range.
/// @param {Real} out_max The maximum of the output range.
function value_map(value, in_min, in_max, out_min, out_max) {
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

function quick_sort(array, cmp_func_or_type, l = -1, r = -1) {
	if(!is_method(cmp_func_or_type)) cmp_func_or_type = bool(cmp_func_or_type);
	if (l == -1 && r == -1) {
		l = 0;
		r = array_length(array) - 1;
	}
	if (l >= r) return;
	var pivot = array[(l + r) >> 1];
	var i = l, j = r, t;
	while (i <= j) {
		if(is_bool(cmp_func_or_type)) {
			if(cmp_func_or_type) {
				while(array[i] > pivot) i++;
				while(array[j] < pivot) j--;
			}
			else {
				while(array[i] < pivot) i++;
				while(array[j] > pivot) j--;
			}
		}
		else {
			while (cmp_func_or_type(array[i], pivot) < 0) i++;
			while (cmp_func_or_type(array[j], pivot) > 0) j--;
		}
		if (i <= j) {
			t = array[i];
			array[i] = array[j];
			array[j] = t;
			i++;
			j--;
		}
	}

	quick_sort(array, cmp_func_or_type, l, j);
	quick_sort(array, cmp_func_or_type, i, r);
}

/// @description Using extern extension to speed up the index sorting.
/// @param {Array} array The array to sort.
/// @param {Function} extract_value_func The function to extract the value for sorting. Should return a real.
/// @returns {Array} The sorted array.
function extern_index_sort(array, extract_value_func) {
	static buff = -1;
	var buffTargetSize = array_length(array) * (8+8);
	if(!buffer_exists(buff) || buffer_get_size(buff) < buffTargetSize) {
		if(!buffer_exists(buff))
			buff = buffer_create(buffTargetSize * 2, buffer_fixed, 1);
		else
			buffer_resize(buff, buffTargetSize * 2);
	}
	buffer_seek(buff, buffer_seek_start, 0);
	for(var i=0, l=array_length(array); i<l; i++) {
		buffer_write(buff, buffer_f64, extract_value_func(array[i]));
		buffer_write(buff, buffer_f64, i);
	}

	DyCore_index_sort(buffer_get_address(buff), array_length(array));

	buffer_seek(buff, buffer_seek_start, 8);
	var _newArray = array_create(array_length(array));
	for(var i=0, l=array_length(array); i<l; i++) {
		var _index = buffer_read(buff, buffer_f64);
		_newArray[i] = array[_index];
		buffer_seek(buff, buffer_seek_relative, 8);
	}

	return _newArray;
}

/// @description Using extern extension to speed up the quick sorting.
/// @param {Array} array The array to sort.
/// @param {Boolean} type The sorting type. True for ascending, false for descending.
function extern_quick_sort(array, type) {
	static buff = -1;
	var buffTargetSize = array_length(array) * 8;
	if(!buffer_exists(buff) || buffer_get_size(buff) < buffTargetSize) {
		if(!buffer_exists(buff))
			buff = buffer_create(buffTargetSize * 2, buffer_fixed, 1);
		else
			buffer_resize(buff, buffTargetSize * 2);
	}
	buffer_seek(buff, buffer_seek_start, 0);
	for(var i=0, l=array_length(array); i<l; i++) {
		buffer_write(buff, buffer_f64, array[i]);
	}

	DyCore_quick_sort(buffer_get_address(buff), array_length(array), type);

	buffer_seek(buff, buffer_seek_start, 0);
	for(var i=0, l=array_length(array); i<l; i++) {
		array[i] = buffer_read(buff, buffer_f64);
	}
}

function is_relative_path(path) {
	return filename_name(path) == path;
}

function file_get_size(file) {
	var f = file_bin_open(file, 0);
	if(f == -1) throw $"File not found: {file}";
	var s = file_bin_size(f);
	file_bin_close(f);
	return s;
}

/// @description Specifically for manually setting the view size for surface. Viewport is not changed.
function manually_set_view_size(width, height) {
	matrix_stack_push(matrix_get(matrix_view));
	matrix_stack_push(matrix_get(matrix_projection));
    matrix_set(matrix_view, matrix_build_lookat(
        width / 2, height / 2, -36000, width / 2, height / 2, 0, 0, 1, 0
    ));
    matrix_set(matrix_projection, matrix_multiply( 
          matrix_build_projection_ortho(width, height, 0.001, 36000),
         [1, 0, 0, 0, 
          0, -1, 0, 0,
          0, 0, 1, 0,
          0, 0, 0, 1]));
}

function manually_reset_view_size() {
	matrix_set(matrix_projection, matrix_stack_top());
	matrix_stack_pop();
	matrix_set(matrix_view, matrix_stack_top());
	matrix_stack_pop();
}