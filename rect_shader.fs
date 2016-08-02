#version 410

uniform vec4 color;
uniform sampler2D tex;
uniform int mode;

uniform vec2 size;
in vec2 uv;
out vec4 fragcolor;

void main(){
  if(mode == 1){
    vec4 col = texture(tex, uv);
    if(col.x >=1 && col.y >= 1 && col.z >=1 && col.a >= 1)
      discard;
    fragcolor = col;
  }else{
    fragcolor = vec4(color);
  }  
}
