#version 410

uniform vec2 offset;
uniform vec2 size;
uniform vec2 window_size;
uniform vec2 uv_offset;
uniform vec2 uv_size;

smooth out vec2 fragment;
out vec2 uv;
void main(){
  int x = gl_VertexID % 2;
  int y = gl_VertexID / 2;
  vec2 f = vec2(x - 0.5, y - 0.5) * size;
  fragment = f;
  uv = uv_offset + vec2(x, y) * uv_size;
  vec2 vertpos = (offset + vec2(x, y) * size) / window_size;
  vertpos = (vertpos - 0.5) * 2.0;
  vertpos.y = vertpos.y * -1;
  gl_Position = vec4(vertpos, 0, 1);
}
