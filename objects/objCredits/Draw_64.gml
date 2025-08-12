
var nh = surface_get_height(application_surface);
var scalar = nh / BASE_RES_H;
var surfW = width * scalar;
var surfH = height * scalar;
var creditsSurf = surface_create(surfW, surfH);

surface_set_target(creditsSurf);
    manually_set_view_size(width, height);
    draw_clear_alpha(c_black, 0);
    var _ele = scribble(i18n_get(text))
        .starting_format(font, c_white)
        .align(fa_left, fa_top)
        .msdf_shadow(c_black, 0.6, 0, 3, 3)
        .blend(image_blend, image_alpha)
    	.transform(scl, scl);
    _ele.draw(nowX, round(nowY));
    if(nowY + _ele.get_height() < - 50)
        nowY = height+50;
    manually_reset_view_size();
surface_reset_target();

draw_surface_ext(creditsSurf, round(x), round(y), width/surfW, height/surfH, 0, c_white, 1);

surface_free(creditsSurf);