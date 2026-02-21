
// Draw Loading.
scribble("[delay,200]Loading...")
    .starting_format("mDynamix", c_white)
    .scale(1.5)
    .align(fa_left, fa_bottom)
    .msdf_shadow(c_dkgray, 0.7, 0, 2, 3)
    .draw(40, BASE_RES_H - 30, typist);

// Draw FMOD sprite as FMOD Attribution demands.
var _width = sprite_get_width(sprFMOD);
var _height = sprite_get_height(sprFMOD);
var _scl = 1.5;
var _logoX = BASE_RES_W - 30 - _width * _scl;
var _logoY = BASE_RES_H - 30 - _height * _scl;

draw_sprite_ext(sprFMOD, 0, _logoX, 
    _logoY + 3, 
    _scl, _scl, 0, c_black, logoAlpha * 0.75);
draw_sprite_ext(sprFMOD, 0, _logoX, 
    _logoY, 
    _scl, _scl, 0, c_white, logoAlpha);