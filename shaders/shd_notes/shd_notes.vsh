attribute vec3 in_Position;
attribute vec2 in_TextureCoord;
attribute vec4 in_Colour;
attribute vec2 in_Mapping;

varying vec2 v_vTexcoord;
varying vec4 v_vColour;
varying vec2 v_vMapping;

void main() {
    gl_Position = gm_Matrices[MATRIX_WORLD_VIEW_PROJECTION]
        * vec4(in_Position.xy, 0.0, 1.0);
    v_vTexcoord = in_TextureCoord;
    v_vColour = in_Colour;
    v_vMapping = in_Mapping;
}
