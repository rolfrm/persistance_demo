#version 410
struct circle{
  float x, y;
  //vec2 pos;
  float size;
  float r, g, b;
  //vec3 color;
};

uniform vec2 positions[10];
uniform float sizes[10];
uniform vec3 colors[10];
uniform int cnt;
out vec4 color;

void main(){
  int hits = 0;
  for(int i = 0; i < cnt; i++){
     float d = length(positions[i].xy - gl_FragCoord.xy) - sizes[i];
     if(d < 0){
        color = vec4(colors[i],1);
	//	break;
	hits++;
     } 
  }
  if(hits == 0)
    discard;
  //  color = vec4(gl_FragCoord.x > 10 ? 0 : 1, gl_FragCoord.y > 10 ? 0 : 1, 1 ,1);
} 