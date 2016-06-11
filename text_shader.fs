#version 410

uniform vec4 color;
in vec2 uv;
out vec4 fragcolor;
uniform sampler2D tex;
void main(){
     vec4 col = color;
     col.a *= texture(tex, uv).x;
     fragcolor = col;
}