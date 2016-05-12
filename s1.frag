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
uniform vec2 velocities[10];
uniform int cnt;
out vec4 color;

void main(){
  int hits = 0;
  color = vec4(1,1,1,0);
  for(int i = 0; i < cnt; i++){
     vec2 v1 = positions[i].xy;
     vec2 v2 = v1 - velocities[i];
     float d = 0.0;
     float vl = length(velocities[i]);
     if(vl < 0.001 || dot(gl_FragCoord.xy - v1, velocities[i]) > 0){
       d = length(gl_FragCoord.xy - v1);
       
     }else if (dot(gl_FragCoord.xy - v2, velocities[i]) < 0){
       d = length(gl_FragCoord.xy - v2);
     }else{
       vec2 pdir = vec2(velocities[i].y, -velocities[i].x);
       d = abs(dot(normalize(pdir), v1 - gl_FragCoord.xy));
     }
     d -= sizes[i];
     //float d = min(length(v1 - gl_FragCoord.xy), length(v2 - gl_FragCoord.xy))  - sizes[i];//min(length(v1), length(v2- gl_FragCoord.xy));
     if(d < 1){
       float w = 1;
       if (d > 0)
	 w = 1 - d;
       if(w > color.a)
	 color = vec4(colors[i],w);// * w;
	//	break;
	hits++;
     } 
  }
  if(hits == 0)
    discard;
  //  color = vec4(gl_FragCoord.x > 10 ? 0 : 1, gl_FragCoord.y > 10 ? 0 : 1, 1 ,1);
} 
