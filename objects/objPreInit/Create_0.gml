
display_reset(4, true);

// Prefetch the large dynamic font texture group into vram.
texturegroup_load("texFonts", true);

typist = scribble_typist().in(1, 10);
timer = 0;

logoAlpha = 0;
logoFadeInTime = 0.4;   // in seconds
logoDelayTime = 0.2;