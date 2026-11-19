varying vec2 v_uv;
varying vec4 v_color;
varying vec2 v_mapping;
varying vec4 v_slice;
varying vec2 v_source;

uniform float u_mono;

void main() {
    vec2 uv = v_uv;
    if(v_mapping.y > 0.5) {
        float width = abs(v_mapping.x);
        float scale = v_source.y;
        float left = v_slice.z * scale;
        float right = width - v_slice.w * scale;
        float x = uv.x;
        // Mirrored/rotated strips own the opposite edge of an exact tile join.
        bool backwards = v_mapping.x < 0.0;
        bool tiled = v_mapping.y > 1.5 && v_mapping.y < 2.5;
        if(tiled && scale < 1.0) {
            // The middle has vanished. Clip each border at native texel scale rather than squeezing the diamond.
            if(x > left || (x == left && !backwards)) x = v_source.x - v_slice.w + (x - left);
        } else if(x < left || (x == left && backwards)) {
            x /= scale;
        } else if(x > right || (x == right && !backwards) || right <= left) {
            x = v_source.x - (width - x) / scale;
        } else {
            float middle = v_source.x - v_slice.z - v_slice.w;
            float distance = x - left;
            if(tiled) {
                x = mod(distance, middle);
                if(x == 0.0 && distance > 0.0 && backwards) x = middle;
            } else {
                x = middle * distance / (right - left);
            }
            x += v_slice.z;
        }
        uv.x = v_slice.x + v_slice.y * x;
    } else if(v_mapping.y < 0.0) {
        float phase = fract(uv.y);
        float start = v_mapping.x;
        if(start < 0.0) {
            start = -1.0 - start;
            if(phase == 0.0 && uv.y > 0.0) phase = 1.0;
        }
        uv.y = start - v_mapping.y * (1.0 - phase);
    }
    vec4 color = v_color * texture2D(gm_BaseTexture, uv);
    if(u_mono > 0.5) {
        float lightness = (max(max(color.r, color.g), color.b) + min(min(color.r, color.g), color.b)) * 0.5;
        color.rgb = vec3(lightness * 0.75 + 0.25);
    }
    gl_FragColor = color;
}
