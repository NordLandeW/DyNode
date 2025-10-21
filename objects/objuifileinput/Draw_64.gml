
var _col = theme_get().color;
scriElement
    .gradient(_col, gradAlpha)
    .msdf_shadow(merge_color(c_black, _col, lerp(0, 0.5, gradAlpha)), 0.3, 0, shadowDistance, 3)
    .draw(x, y);

if(input != "") {
    scribble(cjk_prefix()+input)
        .starting_format("fDynamix16", fontColor)
        .align(fa_left, fa_top)
        .gradient(_col, gradAlpha)
        .fit_to_box(maxWidth, scriHeight)
        .msdf_shadow(merge_color(c_black, _col, lerp(0, 0.5, gradAlpha)), 0.3, 0, shadowDistance, 3)
        .draw(x, y+scriHeight);
}
else {
    var _ny = y + scriHeight*2;
    CleanLine(x, _ny, x + maxWidth + lineShootDistance * gradAlpha, _ny)
        .Blend2(merge_color(fontColor, _col, gradAlpha), 1, fontColor, 1)
        .Thickness(4)
        .Cap("round", "round")
        .Draw();
}