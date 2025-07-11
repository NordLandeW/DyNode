
varying vec2 v_vTexcoord;
varying vec4 v_vColour;

uniform float u_hue;
uniform float u_saturation;

vec3 rgb2hsv(vec3 c) {
  vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

  float d = q.x - min(q.w, q.y);
  float e = 1.0e-10;

  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
  vec4 color = texture2D(gm_BaseTexture, v_vTexcoord) * v_vColour;

  vec3 hsv = rgb2hsv(color.rgb);

  hsv.x = fract(hsv.x + u_hue + 1.0 - 0.5138);
  if(hsv.y > 0.05)
    hsv.y = u_saturation;

  vec3 rgb = hsv2rgb(hsv);

  gl_FragColor = vec4(rgb, color.a);
}