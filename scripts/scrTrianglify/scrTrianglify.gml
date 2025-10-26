function trianglify_draw(tri_struct, colors, threshold = 0) {
	var w = tri_struct.width;
	var h = tri_struct.height;

	// Static buffers to avoid leaks when tri_struct lifecycle changes
	static __tri_buf_in = -1;
	static __tri_buf_in_pts = -1;
	static __tri_buf_out = -1;
	static __tri_buf_out_cap = 0;

	// Prepare input buffer (f64)
	var _pts = tri_struct.points;
	var _pcount = array_length(_pts);
	
	if ((__tri_buf_in == -1) || !buffer_exists(__tri_buf_in) || (__tri_buf_in_pts != _pcount)) {
		if ((__tri_buf_in != -1) && buffer_exists(__tri_buf_in)) buffer_delete(__tri_buf_in);
		__tri_buf_in = buffer_create(4 + _pcount * 2 * 8, buffer_grow, 1); // u32 count + 2*f64 per point
		__tri_buf_in_pts = _pcount;
	}
	
	buffer_seek(__tri_buf_in, buffer_seek_start, 0);
	buffer_write(__tri_buf_in, buffer_u32, _pcount);
	for (var i = 0; i < _pcount; i++) {
		buffer_write(__tri_buf_in, buffer_f64, _pts[i].x);
		buffer_write(__tri_buf_in, buffer_f64, _pts[i].y);
	}
	
	// Ensure output buffer capacity (f64 output). Header(u32) + tri*6*f64
	var _tri_upper = max(2 * _pcount, 8);
	var _need_guess = 4 + _tri_upper * 6 * 8;
	if ((__tri_buf_out == -1) || !buffer_exists(__tri_buf_out)) {
		__tri_buf_out_cap = _need_guess;
		__tri_buf_out = buffer_create(__tri_buf_out_cap, buffer_grow, 1);
	} else if (__tri_buf_out_cap < _need_guess) {
		buffer_delete(__tri_buf_out);
		__tri_buf_out_cap = _need_guess;
		__tri_buf_out = buffer_create(__tri_buf_out_cap, buffer_grow, 1);
	}
	
	// Call native triangulation (coordFormat=1 for f64)
	var _ret = DyCore_delaunator_buffer(buffer_get_address(__tri_buf_in), buffer_get_address(__tri_buf_out), __tri_buf_out_cap, 1.0);
	if (_ret < 0) {
		var _need = -_ret;
		buffer_delete(__tri_buf_out);
		__tri_buf_out_cap = _need;
		__tri_buf_out = buffer_create(__tri_buf_out_cap, buffer_grow, 1);
		_ret = DyCore_delaunator_buffer(buffer_get_address(__tri_buf_in), buffer_get_address(__tri_buf_out), __tri_buf_out_cap, 1.0);
		if (_ret < 0) {
			// Failed again; skip drawing this frame.
			return;
		}
	}
	
	var _tri_count = _ret;
	
	// Drawing
	gpu_push_state();
	gpu_set_blendmode_ext(bm_one, bm_zero);
	draw_clear_alpha(c_black, 0);
	draw_set_alpha(1);
	draw_set_color(c_white);
	
	buffer_seek(__tri_buf_out, buffer_seek_start, 0);
	var _read_tri_count = buffer_read(__tri_buf_out, buffer_u32);
	
	draw_primitive_begin(pr_trianglelist);
	for (var t = 0; t < _read_tri_count; t++) {
		var x1 = buffer_read(__tri_buf_out, buffer_f64);
		var y1 = buffer_read(__tri_buf_out, buffer_f64);
		var x2 = buffer_read(__tri_buf_out, buffer_f64);
		var y2 = buffer_read(__tri_buf_out, buffer_f64);
		var x3 = buffer_read(__tri_buf_out, buffer_f64);
		var y3 = buffer_read(__tri_buf_out, buffer_f64);
		
		var cx = (x1 + x2 + x3) / 3;
		var cy = (y1 + y2 + y3) / 3;
		
		var _color = __2corner_color_merge(colors, cx, cy, w, h);
		_color = __trisys_mouselight(_color, cx, cy, colors[2]);
		draw_set_color(_color);
		
		var _alp = cy / h;
		_alp = max(_alp - threshold, 0);
		_alp = lerp(0, 1, _alp / (1 - threshold));
		
		draw_vertex_color(x1, y1, _color, 1);
		draw_vertex_color(x2, y2, _color, 1);
		draw_vertex_color(x3, y3, _color, 1);
	}
	draw_primitive_end();
	gpu_pop_state();
}

function trianglify_generate(w, h, speedRange, cellSize=75, variance=0.75) {
	var _trianglify = {
		width: w,
		height: h,
		points: []
	};
	var colCount = floor(w / cellSize) + 4;
    var rowCount = floor(h / cellSize) + 4;
    var bleedX = ((colCount * cellSize) - w) / 2;
    var bleedY = ((rowCount * cellSize) - h) / 2;
    var cellJitter = cellSize * variance;
    var getJitter = function (c) { return (dyc_random(1) - 0.5) * c; } ;
    var halfCell = cellSize / 2;
    for(var i=0; i<colCount; i++)
    for(var j=0; j<rowCount; j++) {
    	var _corner = !(((i==0)+(i==colCount-1)+(j==0)+(j==rowCount-1)) >= 2);
    	array_push(_trianglify.points, {
    		x : -bleedX + i * cellSize + halfCell + _corner * getJitter(cellJitter),
    		y : -bleedY + j * cellSize + halfCell + _corner * getJitter(cellJitter),
    		vx : (in_between(i, 2, colCount - 3) && in_between(j, 2, rowCount - 3))
    			* dyc_random_range(speedRange[0], speedRange[1]) * (dyc_irandom(1)*2-1)
    			* global.timeManager.get_fps_scale(),
    		vy : (in_between(i, 2, colCount - 3) && in_between(j, 2, rowCount - 3))
    			* dyc_random_range(speedRange[0], speedRange[1]) * (dyc_irandom(1)*2-1)
    			* global.timeManager.get_fps_scale()
    	});
    }
    
    return _trianglify;
}

function trianglify_step(tri_struct) {
	var _arr = tri_struct.points;
	for(var i=0; i<array_length(_arr); i++) {
		if(!in_between(_arr[i].x+_arr[i].vx, 0, tri_struct.width))
			_arr[i].vx = -_arr[i].vx;
		if(!in_between(_arr[i].y+_arr[i].vy, 0, tri_struct.height))
			_arr[i].vy = -_arr[i].vy;
		_arr[i].x += _arr[i].vx;
		_arr[i].y += _arr[i].vy;
	}
}

function __trisys_sparkle(pos, w, h, jitterFactor = 0.15) {
	return [
		pos[0]+(dyc_random(1)-0.5)*jitterFactor*w,
		pos[1]+(dyc_random(1)-0.5)*jitterFactor*h,
		];
}

function __trisys_mouselight(col, x, y, lcol = c_white, radius = 300, alp = 0.4) {
	return merge_color(col, lcol, lerp(0, alp, max(1-point_distance(x, y, mouse_x, mouse_y)/radius, 0)));
}

function __trisys_centroid(str) {
	return [(str.p1[0]+str.p2[0]+str.p3[0])/3, (str.p1[1]+str.p2[1]+str.p3[1])/3];
}

function __2corner_color_merge(colors, x, y, w, h) {
	x = clamp(x, 0, w);
	y = clamp(y, 0, h);
	var _std = sqrt(w*w+h*h);
	var _col = c_black;
	_col = merge_color(_col, colors[0], point_distance(0, 0, x, y)/_std);
	_col = merge_color(_col, colors[1], point_distance(w, h, x, y)/_std);
	return _col;
}