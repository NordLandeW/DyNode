/// @description 

// Inherit the parent event
event_inherited();

function draw_event () {
    if(!drawVisible) return;
    if(global.style == STYLE.DYNAMITE) {
        // The right edge of each half meets its mirror at the centre.
        var _half = dynamite_note_width() / 2 + 0.14 * DYNAMITE_WORLD_PX;
        var _dx = (side == 0) * _half / 2;
        var _dy = ((side == 1) - (side == 2)) * _half / 2;
        dynamite_draw_strip(sprDynamiteSlideHalf, x - _dx, y - _dy, _half, 30, 57, true);
        dynamite_draw_strip(sprDynamiteSlideHalf, x + _dx, y + _dy, -_half, 30, 57, true);
    }
    else {
        draw_sprite_ext(sprChainL, 0, x, y, image_xscale, image_yscale,
                        image_angle, image_blend, image_alpha);
        draw_sprite_ext(sprChainR, 0, x, y, image_xscale, image_yscale,
                        image_angle, image_blend, image_alpha);
    }
}

noteType = 1;

sprite = sprChain;
_prop_init(true);
