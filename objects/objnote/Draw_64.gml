/// Debug Drawing.

if(global.debugGizmos) {

    // Draw bbox.
    draw_set_color_alpha(c_neon_pink, 0.2);
    var _bbox = get_bbox();
    draw_rectangle(_bbox.left, _bbox.top, _bbox.right, _bbox.bottom, false);
    draw_set_color_alpha(c_white, 1);

}