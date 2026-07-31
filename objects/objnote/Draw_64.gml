/// Debug Drawing.

if(global.debugGizmos) {

    // Draw bbox.
    draw_set_color_alpha(c_neon_pink, 0.2);
    draw_rectangle(bbox_left, bbox_top, bbox_right, bbox_bottom, false);
    draw_set_color_alpha(c_white, 1);

}