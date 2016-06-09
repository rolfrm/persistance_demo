#version 410

uniform vec4 color;
in vec2 uv;
out vec4 fragcolor;
uniform sampler2D tex;
void main(){
     fragcolor = texture(tex, uv) * color;
}