/// @description 

// Inherit the parent event
event_inherited();

function draw_event () {
    if(!drawVisible) return;
    draw_sprite_ext(sprChainL, 0, x, y, image_xscale, image_yscale,
                    image_angle, image_blend, image_alpha);
    draw_sprite_ext(sprChainR, 0, x, y, image_xscale, image_yscale,
                    image_angle, image_blend, image_alpha);
}

noteType = 1;

// Correction Values
lFromLeft = 7;
rFromRight = 7;

sprite = sprChain;
_prop_init(true);