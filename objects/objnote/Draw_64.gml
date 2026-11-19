/// Debug Drawing.

if(global.debugGizmos) {

    // Draw bbox.
    draw_set_color_alpha(c_neon_pink, 0.2);
    var _bbox = get_bbox();
    draw_rectangle(_bbox.left, _bbox.top, _bbox.right, _bbox.bottom, false);

    // Show the biased chart span, independent of sprite padding and the pink mouse box.
    if(drawVisible && noteType != NOTE_TYPE.SUB && editor_get_editmode() != 5) {
        var _front = side == NOTE_SIDE.FRONT;
        var _center = note_pos_to_x(position, side);
        var _half = max(0, note_biased_width(width, side)) / 2;
        var _lo = _center - _half, _hi = _center + _half;
        var _from = _front ? y : x, _to = _from;
        if(noteType == NOTE_TYPE.HOLD) {
            _to = note_time_to_y(note_exists(sinst) ? sinst.time : time + lastTime, side);
        }
        draw_set_color_alpha(c_lime, 0.85);
        if(_front) {
            draw_rectangle(_lo, min(_from, _to), _hi, max(_from, _to), true);
            draw_line(_lo, _from - 5, _lo, _from + 5);
            draw_line(_hi, _from - 5, _hi, _from + 5);
        }
        else {
            draw_rectangle(min(_from, _to), _lo, max(_from, _to), _hi, true);
            draw_line(_from - 5, _lo, _from + 5, _lo);
            draw_line(_from - 5, _hi, _from + 5, _hi);
        }
    }
    draw_set_color_alpha(c_white, 1);

}