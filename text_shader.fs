#version 410

uniform vec4 color;
in vec2 uv;
out vec4 fragcolor;
uniform sampler2D tex;
void main(){
     vec4 col = texture(tex, uv * vec2(0.2, 0.1)) * color +vec4(0.1);
     fragcolor = col;
}