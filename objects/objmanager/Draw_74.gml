/// @description Finish the preview and capture after the shared scene has reached the display.
if(!global.recordManager.recording) exit;

if(global.style == STYLE.DYNAMITE) {
    with(objMain) {
        if(visible && standardAlpha > 0) {
            dynamite_draw_auto_badge(standardAlpha * autoBadgeClearAlpha);

            // The GUI is screen-only. The encoder reads this surface without announcements.
            surface_set_target(application_surface);
            dynamite_draw_auto_badge(standardAlpha);
            surface_reset_target();
        }
    }
}
global.recordManager.push_frame();
