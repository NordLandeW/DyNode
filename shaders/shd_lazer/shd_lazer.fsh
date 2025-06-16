//
// Simple passthrough fragment shader
//
varying vec2 v_vTexcoord;
varying vec4 v_vColour;

uniform float mean;
uniform float stdDev;

void main() {
  vec2 uv = v_vTexcoord;
  vec4 base_col = v_vColour * texture2D(gm_BaseTexture, v_vTexcoord);

  // Normal Distribution Drop
  float maxHeight = 1.0;  // Maximum brightness

  // Gaussian function: f(x) = maxHeight * e^(-(x-mean)^2 / (2*stdDev^2))
  float bri =
      maxHeight * exp(-pow(uv.y - mean, 2.0) / (2.0 * pow(stdDev, 2.0)));
  base_col = vec4(1., 1., 1., bri);
  // base_col *= v_vColour;

  gl_FragColor = base_col;
}
