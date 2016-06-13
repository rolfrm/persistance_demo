#version 410

uniform vec2 offset;
uniform vec2 size;
uniform vec2 window_size;
uniform vec4 roundness;
smooth out vec2 fragment;
void main(){
  int x = gl_VertexID % 2;
  int y = gl_VertexID / 2;
  vec2 f = vec2(x - 0.5, y - 0.5) * size;
  fragment = f;
  
  //  csize = 
  //corner = vec2(x == 0 ? roundness.x : size.x - roundness.z, y == 0 ? roundness.y : size.y - roundness.w);
  vec2 vertpos = (offset + vec2(x, y) * size) / window_size;
  vertpos = (vertpos - 0.5) * 2.0;
  vertpos.y = vertpos.y * -1;
  gl_Position = vec4(vertpos, 0, 1);
}
