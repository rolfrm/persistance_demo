#version 410

uniform mat4 camera;
uniform float depth;
in vec2 vertex;

void main(){
     vec4 p = camera * vec4(vertex, 0, 1);
     p.z = depth * 0.001;
       gl_Position = p;
}
