//
// Simple passthrough fragment shader
//
varying vec2 v_vTexcoord;
varying vec4 v_vColour;

void main() {
    vec4 col = texture2D(gm_BaseTexture, v_vTexcoord);

    col.rgba = vec4(col.bgr, 1.0) * v_vColour;

    gl_FragColor = col;
}
