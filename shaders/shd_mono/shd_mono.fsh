varying vec2 v_vTexcoord;
varying vec4 v_vColour;

vec3 rgb2hsl(vec3 c) {
  float r = c.r, g = c.g, b = c.b;
  float maxc = max(max(r, g), b);
  float minc = min(min(r, g), b);
  float h = 0.0, s = 0.0, l = (maxc + minc) * 0.5;

  if (maxc != minc) {
    float d = maxc - minc;
    s = l < 0.5 ? d / (maxc + minc) : d / (2.0 - maxc - minc);
    if (maxc == r)
      h = (g - b) / d + (g < b ? 6.0 : 0.0);
    else if (maxc == g)
      h = (b - r) / d + 2.0;
    else
      h = (r - g) / d + 4.0;
    h /= 6.0;
  }
  return vec3(h, s, l);
}

float hue2rgb(float p, float q, float t) {
  if (t < 0.0) t += 1.0;
  if (t > 1.0) t -= 1.0;
  if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
  if (t < 1.0 / 2.0) return q;
  if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
  return p;
}

vec3 hsl2rgb(vec3 hsl) {
  float h = hsl.x;
  float s = hsl.y;
  float l = hsl.z;

  float r, g, b;

  if (s == 0.0) {
    r = g = b = l;  // achromatic: grayscale
  } else {
    float q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
    float p = 2.0 * l - q;
    r = hue2rgb(p, q, h + 1.0 / 3.0);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1.0 / 3.0);
  }

  return vec3(r, g, b);
}

void main() {
  vec4 col = v_vColour * texture2D(gm_BaseTexture, v_vTexcoord);
  vec3 hsl = rgb2hsl(col.rgb);

  // Force to grayscale: no color, only lightness
  hsl.x = 0.0;  // hue doesn't matter when saturation = 0
  hsl.y = 0.0;

  hsl.z = hsl.z * 0.75 + 0.25;

  vec3 bwColor = hsl2rgb(hsl);
  gl_FragColor = vec4(bwColor, col.a);
}
