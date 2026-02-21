
var nh = surface_get_height(application_surface);
var scalar = nh / BASE_RES_H;
var surfW = width * scalar;
var surfH = height * scalar;
var creditsSurf = surface_create(surfW, surfH);
var text = get_about_text();

surface_set_target(creditsSurf);
    manually_set_view_size(width, height);
    draw_clear_alpha(c_black, 0);

    var _ele = scribble(middleText)
        .starting_format("sprMsdfNotoSans", c_white)
        .align(fa_middle, fa_top)
        .msdf_shadow(c_black, 0.8, 0, 3, 6)
        .blend(image_blend, image_alpha);
    _ele.draw(width / 2, round(nowY));

    var _license_ele = scribble(rightText)
        .starting_format("sprMsdfNotoSans", c_white)
        .align(fa_right, fa_top)
        .msdf_shadow(c_black, 0.8, 0, 3, 6)
        .blend(image_blend, image_alpha);
    _license_ele.draw(width - 100, round(nowY + _ele.get_height() + 40));

    var _ele_total_height = _ele.get_height() + 40 + _license_ele.get_height();
    if(nowY + _ele_total_height < - 50) {
        nowY = height + 50;
        if(abs(baseSpeed - currentSpeed) < 10)
            currentLoopAcceleration = loopAcceleration;
    }
    else if(nowY > height + 50) {
        nowY = -_ele_total_height;
    }
    manually_reset_view_size();
surface_reset_target();

draw_surface_ext(creditsSurf, 0, 0, width/surfW, height/surfH, 0, c_white, 1);

surface_free(creditsSurf);

if(DEBUG_MODE) {
    var dbgText = $"nowY: {string_format(nowY, 0, 2)}\n";
    dbgText += $"currentAcceleration: {string_format(currentAcceleration, 0, 2)}\n";
    dbgText += $"currentSpeed: {string_format(currentSpeed, 0, 2)}\n";
    draw_set_color(c_red);
    draw_text(10, 10, dbgText);
}