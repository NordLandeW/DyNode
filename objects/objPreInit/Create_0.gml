
display_reset(2, true);

// Prefetch the large dynamic font texture group into vram.
texturegroup_load("texFonts", true);

typist = scribble_typist().in(1, 10);