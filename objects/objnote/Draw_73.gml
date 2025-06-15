/// @description Draw editor & debug things

if((drawVisible || nodeAlpha > EPS || infoAlpha > EPS) && editor_get_editmode() <= 4) {
    var _inv = noteType == 3 ? -1:1;
	if(nodeAlpha>EPS) {
		CleanRectangleXYWH(x, y, nodeRadius, nodeRadius)
			.Rounding(5)
			.Blend(merge_color(
				nodeColor, 
				unselectHint ? scribble_rgb_to_bgr(0xff1744) : c_fuchsia,
				nodeBorderAlpha * 0.8), 
				nodeAlpha)
			.Draw();
	}
    
    // Draw Information
    if(infoAlpha > EPS) {
    	var _dx = 20, _dy = (noteType == 2? dFromBottom:20) * _inv,
    		_dyu = (noteType == 2? dFromBottom:25) * _inv;
	    scribble(string_format(position, 1, 2))
	    	.starting_format("fDynamix16", c_aqua)
	    	.transform(global.scaleXAdjust, global.scaleYAdjust)
	    	.blend(c_white, infoAlpha)
	    	.align(fa_right, fa_middle)
	    	.draw(x - _dx, y + _dy);
	    
	    scribble(string_format(width, 1, 2))
	    	.starting_format("fDynamix16", c_white)
	    	.transform(global.scaleXAdjust, global.scaleYAdjust)
	    	.blend(c_white, infoAlpha)
	    	.align(fa_left, fa_middle)
	    	.draw(x + _dx, y + _dy);
	    
	    var _time = objMain.showBar ? "Bar " + string_format(time_to_bar_dyn(time), 1, 5) : string_format(time, 1, 0) + " ms";
	    scribble(_time)
	    	.starting_format("fDynamix16", scribble_rgb_to_bgr(0xb2fab4))
	    	.transform(global.scaleXAdjust, global.scaleYAdjust)
	    	.blend(c_white, infoAlpha)
	    	.align(fa_right, fa_middle)
	    	.draw(x - _dx, y - _dyu);
	    
	    if(is_struct(lastAttachBar) && lastAttachBar.bar != undefined) {
	    	var _bar = string_format(lastAttachBar.bar, 1, 0)+" + "+string_format(lastAttachBar.diva, 1, 0)+"/"+string(lastAttachBar.divb)
	    	_bar += " [#FFAB91]("+string(lastAttachBar.divc)+")"
	    	scribble(_bar)
		    	.starting_format("fDynamix16", scribble_rgb_to_bgr(0xFFE082))
		    	.transform(global.scaleXAdjust, global.scaleYAdjust)
		    	.blend(c_white, infoAlpha)
		    	.align(fa_left, fa_middle)
		    	.draw(x + _dx, y - _dyu);
	    }
    }
    
}
else animTargetNodeA = 0;