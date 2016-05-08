#version 410

void main(){
  int x = gl_VertexID % 2;
  int y = gl_VertexID / 2;    
  gl_Position = vec4(y * 2 - 1, x * 2 - 1 , 0, 1);
}