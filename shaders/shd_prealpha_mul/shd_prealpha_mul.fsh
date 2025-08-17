//
// Simple passthrough fragment shader
//
varying vec2 v_vTexcoord;
varying vec4 v_vColour;

void main() {
  vec4 inCol = vec4(v_vColour.rgb * v_vColour.a, v_vColour.a);
  vec4 col = inCol * texture2D(gm_BaseTexture, v_vTexcoord);
  gl_FragColor = col;
}
