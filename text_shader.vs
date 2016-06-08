#version 410

uniform vec2 offset;
uniform vec2 size;
uniform vec2 window_size;

out vec2 uv;
void main(){
  int x = gl_VertexID % 2;
  int y = gl_VertexID / 2;
  uv = vec2(x, 1 - y);
  vec2 vertpos = (offset + vec2(x, y) * size) / window_size ;

  gl_Position = vec4((vertpos - vec2(0.5,0.5)) * 2.0, 0, 1);
}
