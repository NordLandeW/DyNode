
#macro ANNO_BORDER_ANIMATION_TIME 1500

str = ""
lastTime = 0
uniqueID = ""

enum ANNO_STATE {
	IDLE,
	PROCESSING,
	COMPLETE,
	ERROR
};

enum ANNO_TYPE {
	NORMAL,
	TASK
};

annoType = ANNO_TYPE.NORMAL;
annoState = ANNO_STATE.IDLE;

_generate_element = function () {
	element = scribble(cjk_prefix() + str)
		.wrap(0.7 * BASE_RES_W, -1, i18n_get_lang()=="en-us" ? false: true)
		.align(fa_right, fa_bottom)
		.transform(0.8, 0.8);
}

init_color = function() {
	update_color();
	bgColor = targetBgColor.copy();
}

update_color = function() {
	var _col = theme_get().color;
	switch(annoState) {
		case ANNO_STATE.ERROR:
			_col = merge_color(c_red, c_white, 0.5);
			break;
		case ANNO_STATE.COMPLETE:
			_col = merge_color(c_green, c_white, 0.5);
			break;
		default:
			break;
	}
	targetBgColor.set_color(_col);
}

init = function(str, lastTime, uniqueID, type = ANNO_TYPE.NORMAL) {
	self.str = str;
	self.lastTime = lastTime;
	self.uniqueID = uniqueID;
	self.annoType = type;
	if(type == ANNO_TYPE.TASK) self.annoState = ANNO_STATE.PROCESSING;
	_generate_element();
}

/// @param {Enum.ANNO_STATE} state 
set_state = function(state) {
	annoState = state;
	timer = 0;
}

image_alpha = 0;
animTargetAlpha = 1;

shiftInX = -50;
shiftTime = 500;
shiftCurv = animcurve_get_channel(curvShiftIn, "curve1");
timer = 0;
anniTimer = 0;
targetY = 0;
nowShiftY = 0;

bgColor = new HPColor(255, 255, 255);
targetBgColor = bgColor.copy();
borderThick = 0;
borderAlpha = 0;
