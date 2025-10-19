
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