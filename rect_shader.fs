#version 410

uniform vec4 color;

in vec2 fragment;
uniform vec2 size;

out vec4 fragcolor;

float udRoundBox( vec2 p, vec2 b, float r )
{
  return length(max(abs(p)-b, 0.0))-r;
}



void main(){

  if(length(abs(fragment) - 3) - 10 > 0)
    discard;
  //if(udRoundBox(fragment,size * 0.5, 10) > 0)
  //  discard;
  
  fragcolor = color;
  
  fragcolor.r = abs(fragment).x - size.x;
  //fragcolor.y = udRoundBox(fragment, size * 0.5, 0.0);
  //fragcolor.x = fragment.x;
}
