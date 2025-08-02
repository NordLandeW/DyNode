function trianglify_draw(tri_struct, colors, threshold = 0) {
	var w = tri_struct.width;
	var h = tri_struct.height;
	var _temp = [];
	
	for(var i=0; i<array_length(tri_struct.points); i++)
		array_push(_temp, tri_struct.points[i].x, tri_struct.points[i].y);

	gpu_push_state();
	gpu_set_blendmode_ext(bm_one, bm_zero);
	draw_clear_alpha(c_black, 0);
	draw_set_alpha(1);
	// Triangulation
	var _arr = json_parse(DyCore_delaunator(json_stringify(_temp)));
	draw_set_color(c_white);
	draw_primitive_begin(pr_trianglelist);
	for(var i=0; i<array_length(_arr); i++) {
		_arr[i].c = __trisys_centroid(_arr[i]);
		// Sparkle Effect
		// _arr[i].c = __trisys_spakle(_arr[i].c, w, h);
		
		var _color = __2corner_color_merge(colors, _arr[i].c[0], _arr[i].c[1], w, h);
		_color = __trisys_mouselight(_color, _arr[i].c[0], _arr[i].c[1], colors[2]);
		draw_set_color(_color);
		var _alp = _arr[i].c[1]/h;
		_alp = max(_alp - threshold, 0);
		_alp = lerp(0, 1, _alp/(1-threshold));
		draw_vertex_color(_arr[i].p1[0], _arr[i].p1[1], _color, 1);
		draw_vertex_color(_arr[i].p2[0], _arr[i].p2[1], _color, 1);
		draw_vertex_color(_arr[i].p3[0], _arr[i].p3[1], _color, 1);
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
    var getJitter = function (c) { return (random(1) - 0.5) * c; } ;
    var halfCell = cellSize / 2;
    for(var i=0; i<colCount; i++)
    for(var j=0; j<rowCount; j++) {
    	var _corner = !(((i==0)+(i==colCount-1)+(j==0)+(j==rowCount-1)) >= 2);
    	array_push(_trianglify.points, {
    		x : -bleedX + i * cellSize + halfCell + _corner * getJitter(cellJitter),
    		y : -bleedY + j * cellSize + halfCell + _corner * getJitter(cellJitter),
    		vx : (in_between(i, 2, colCount - 3) && in_between(j, 2, rowCount - 3))
    			* random_range(speedRange[0], speedRange[1]) * (irandom(1)*2-1)
    			* global.fpsAdjust,
    		vy : (in_between(i, 2, colCount - 3) && in_between(j, 2, rowCount - 3))
    			* random_range(speedRange[0], speedRange[1]) * (irandom(1)*2-1)
    			* global.fpsAdjust
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
		pos[0]+(random(1)-0.5)*jitterFactor*w,
		pos[1]+(random(1)-0.5)*jitterFactor*h,
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