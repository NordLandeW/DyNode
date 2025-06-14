//
// Simple passthrough fragment shader
//
varying vec2 v_vTexcoord;
varying vec4 v_vColour;

void main() {
  vec4 col = v_vColour * texture2D(gm_BaseTexture, v_vTexcoord);
  if (col.a > 0.) col = vec4(col.rgb / col.a, col.a);
  // col.a = 1.;
  gl_FragColor = col;
}
