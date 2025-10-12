
str = ""
lastTime = 0
uniqueID = ""

_generate_element = function () {
	element = scribble(cjk_prefix() + str)
		.wrap(0.7 * BASE_RES_W, -1, i18n_get_lang()=="en-us" ? false: true)
		.align(fa_right, fa_bottom)
		.transform(0.8, 0.8);
}

init = function(str, lastTime, uniqueID) {
	self.str = str;
	self.lastTime = lastTime;
	self.uniqueID = uniqueID;
	_generate_element();
}

image_alpha = 0;
animTargetAlpha = 1;

shiftInX = -50;
shiftTime = 500;
shiftCurv = animcurve_get_channel(curvShiftIn, "curve1");
timer = 0;
targetY = 0;
nowShiftY = 0;