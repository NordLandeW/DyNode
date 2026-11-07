varying vec2 v_vTexcoord;
varying vec4 v_vColour;
varying vec2 v_vMapping;

uniform vec4 u_note_uv;
uniform vec4 u_chain_uv;
uniform vec4 u_hold_uv;
uniform vec4 u_edge_uv;
uniform vec3 u_edge_segments; // Left, right, source width.
uniform vec3 u_note_segments;  // Left, right, source width.
uniform vec4 u_chain_segments; // Left, center, right, source width.
uniform float u_mono;

vec2 atlas_uv(vec4 rect, vec2 uv) {
    return mix(rect.xy, rect.zw, uv);
}

float map_three_segment_x(float x, float width, vec3 segments) {
    float left = segments.x;
    float right = segments.y;
    float source = segments.z;
    if(x < left) return x;
    if(x >= width - right) return source - (width - x);
    float span = width - left - right;
    return left + (x - left) * (source - left - right) / span;
}

float map_chain_x(float x, float width) {
    float left = u_chain_segments.x;
    float center = u_chain_segments.y;
    float right = u_chain_segments.z;
    float source = u_chain_segments.w;
    float span = (width - left - center - right) * 0.5;
    float sourceSpan = (source - left - center - right) * 0.5;
    if(x < left) return x;
    if(x < left + span)
        return left + (x - left) * sourceSpan / span;
    if(x < left + span + center)
        return left + sourceSpan + x - left - span;
    if(x < width - right)
        return left + sourceSpan + center
            + (x - left - span - center) * sourceSpan / span;
    return source - (width - x);
}

void main() {
    vec2 uv = v_vTexcoord;
    float kind = v_vMapping.y;
    if(kind > 0.5) {
        if(kind < 1.5) {
            float x = map_three_segment_x(uv.x, v_vMapping.x, u_note_segments);
            uv = vec2(u_note_uv.x + x * ((u_note_uv.z - u_note_uv.x) / u_note_segments.z),
                mix(u_note_uv.y, u_note_uv.w, uv.y));
        } else if(kind < 2.5) {
            float x = map_chain_x(uv.x, v_vMapping.x);
            uv = vec2(u_chain_uv.x + x * ((u_chain_uv.z - u_chain_uv.x) / u_chain_segments.w),
                mix(u_chain_uv.y, u_chain_uv.w, uv.y));
        } else if(kind < 3.5) {
            float x = map_three_segment_x(uv.x, v_vMapping.x, u_edge_segments);
            // Top/bottom strips already carry atlas Y; only X is segmented.
            uv.x = u_edge_uv.x + x * ((u_edge_uv.z - u_edge_uv.x) / u_edge_segments.z);
        } else {
            float phase = fract(uv.y);
            // A left-facing tile includes its ending edge under the top-left rule.
            if(phase == 0.0 && uv.y > 0.0 && v_vMapping.x > 0.5) phase = 1.0;
            uv = atlas_uv(u_hold_uv, vec2(uv.x, phase));
        }
    }
    vec4 color = v_vColour * texture2D(gm_BaseTexture, uv);
    if(u_mono > 0.5) {
        // Same HSL lightness and lift as shd_mono, with saturation set to zero.
        float lightness = (max(max(color.r, color.g), color.b)
            + min(min(color.r, color.g), color.b)) * 0.5;
        color.rgb = vec3(lightness * 0.75 + 0.25);
    }
    gl_FragColor = color;
}
