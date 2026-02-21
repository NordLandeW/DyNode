

if(texturegroup_get_status("texFonts") == texturegroup_status_fetched) {
    instance_destroy();
    instance_create_depth(0, 0, 0, objManager);
}

if(delta_time < 1/20 * 1000000) {
    timer += delta_time / 1000000;
}
logoAlpha = clamp((timer - logoDelayTime) / logoFadeInTime, 0, 1);