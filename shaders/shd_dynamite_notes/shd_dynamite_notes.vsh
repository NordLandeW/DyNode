attribute vec3 in_Position;
attribute vec2 in_TextureCoord;
attribute vec4 in_Colour;
attribute vec2 in_Mapping;

// Atlas U origin, U per source texel, left cut and right cut.
uniform vec4 u_tap_slice;
uniform vec4 u_slide_slice;
uniform vec4 u_up_slice;
uniform vec4 u_down_slice;
uniform vec4 u_slice_widths;

varying vec2 v_uv;
varying vec4 v_color;
varying vec2 v_mapping;
varying vec4 v_slice;
varying vec2 v_source; // Source width and scale of fixed end caps.

void main() {
    gl_Position = gm_Matrices[MATRIX_WORLD_VIEW_PROJECTION] * vec4(in_Position.xy, 0.0, 1.0);
    v_uv = in_TextureCoord;
    v_color = in_Colour;
    v_mapping = in_Mapping;
    vec4 slice = u_tap_slice;
    float width = u_slice_widths.x;
    if(in_Mapping.y > 3.5) {
        slice = u_down_slice;
        width = u_slice_widths.w;
    } else if(in_Mapping.y > 2.5) {
        slice = u_up_slice;
        width = u_slice_widths.z;
    } else if(in_Mapping.y > 1.5) {
        slice = u_slide_slice;
        width = u_slice_widths.y;
    }
    v_slice = slice;
    v_source = vec2(width, min(1.0, abs(in_Mapping.x) / max(1.0, slice.z + slice.w)));
}
