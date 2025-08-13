
var _col = theme_get().color;
var _cont = ["L", "D", "R", "DUAL"];

CleanRectangle(x - sideButtonWidth * 1.7, y - 30, x + sideButtonWidth * 1.7, y + 30)
    .Blend(c_black, 0.5 * alpha)
    .Rounding(10)
    .Draw();

CleanRectangle(x - sideButtonWidth * 0.7, y - 30*3 - upButtonPadding, x + sideButtonWidth * 0.7, y - 30 - upButtonPadding)
.Blend(c_black, 0.5 * alpha)
    .Rounding(10)
    .Draw();

for(var i=0; i<3; i++) {
    scribble(_cont[i])
        .starting_format("mDynamix", c_white)
        .gradient(_col, gradAlpha[i])
        .msdf_shadow(_col, gradAlpha[i]*0.3, 0, gradAlpha[i]*shadowDistance)
        .align(fa_middle, fa_center)
        .blend(c_white, alpha)
        .draw(x + (i-1) * sideButtonWidth, y);
}

scribble(_cont[3])
    .starting_format("mDynamix", c_white)
    .gradient(c_red, gradAlpha[i])
    .msdf_shadow(c_red, gradAlpha[i]*0.3, 0, gradAlpha[i]*shadowDistance)
    .align(fa_middle, fa_center)
    .blend(c_white, alpha)
    .draw(x, y - 60 - upButtonPadding);

draw_set_alpha(1);

surface_free_f(bgSurf);