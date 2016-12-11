#version 410

uniform mat4 camera;

in vec2 vertex;

void main(){
  gl_Position = camera * vec4(vertex, 0, 1);
}
