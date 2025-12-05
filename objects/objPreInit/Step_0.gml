

if(texturegroup_get_status("texFonts") == texturegroup_status_fetched) {
    instance_destroy();
    instance_create_depth(0, 0, 0, objManager);
}