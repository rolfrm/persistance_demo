#include <GL/glew.h>
#include <iron/full.h>
#include "game.h"
#include "persist.h"
#include "shader_utils.h"

vec2 vec2_rand(){
  return vec2_new((rand() % 1000) * 0.001, (rand() % 1000) * 0.001);
}

vec2 vec2_rot90(vec2 r){
  return vec2_new(r.y, - r.x);
}

typedef struct{
  vec2 offset;
  int btn1_last;
}map_pos;

void game_loop(){
  
  circle * circles = persist_alloc("game", sizeof(circle) * 40);
  
  map_pos * mpos = persist_alloc("map_pos", sizeof(map_pos));
  controller_state * ctrl = persist_alloc("controller", sizeof(controller_state));
  mpos->offset = vec2_add(mpos->offset, vec2_scale(ctrl->axis.xy, 10));
  
  u64 n_circles = persist_size(circles) / sizeof(circle);
  
  for(u64 i = 0; i < n_circles; i++){
    if(!circles[i].active){
      int kind;
      if(i == 0){
	kind = kind_player;
      }else{
	kind = rand() % (kind_max - 1) + 1;
      }
    
      circles[i].pos = vec2_add(circles[0].pos, vec2_new((rand() % 500) - 250, (rand() % 500) - 250));
      circles[i].active = true;
      circles[i].size = rand() % 50 + 10;
      circles[i].color = vec3_new(1,0,1);
      circles[i].color.xy = vec2_rand();
      circles[i].vel = vec2_zero;
      if(kind == kind_wall){
	circles[i].color = vec3_new(0.2,0.2,0.2);
      }else if (kind == kind_coin){
	circles[i].color = vec3_new(0.8,0.8,0.5);
      }else if (kind == kind_player){
	circles[i].size = 5;
      }
      circles[i].kind = kind;
    }
    
    circles[i].pos = vec2_add(circles[i].pos,circles[i].vel);
    if(i != 0){
      if(circles[i].kind != kind_wall){
	circles[i].vel = vec2_scale( circles[i].vel, 0.98);
      }
    }else{
      circles[i].vel = vec2_scale( vec2_add(circles[i].vel, vec2_scale(ctrl->axis.xy, 1)), 0.98);
    }
    
  }
  
  if(ctrl->btn1_clicks != mpos->btn1_last){
    mpos->btn1_last = ctrl->btn1_clicks;
    circles[0].color = vec3_new(1.0, 0.6, 0.4);
    n_circles += 1;
    
    circles = persist_realloc(circles, sizeof(circle) * (n_circles));
    logd("N circles: %i\n", n_circles);
  }else{
    circles[0].color = vec3_new(0.4, 0.6, 0.4);
  }

  int n_coins = 0;
  vec2 c0pos = circles[0].pos;
  float c0size = circles[0].size;
  for(u64 i = 1; i < n_circles; i++){
    circle * c = circles + i;
    if(c->kind != kind_coin)
      continue;
    n_coins++;
    float d = vec2_len(vec2_sub(c->pos, c0pos)) - c->size - c0size;
    if(d < 0){
      c->active = false;
      circles[i].color = vec3_new(0.3,0.3,0.3);
    }
  }
  
  //logd("Coins: %i\n", n_coins);
  for(u64 i = 0; i < n_circles; i++){
    circle * c0 = circles + i;
    if(c0->kind == kind_coin)
      continue;
    f32 c0_mass = c0->size;
    if(c0->kind == kind_wall)
      c0_mass = f32_infinity;
    for(u64 j = i + 1; j < n_circles; j++){
      circle * c1 = circles + j;
      if(c1->kind == kind_coin)
	continue;
      f32 c1_mass = c1->size;
      if(c1->kind == kind_wall)
	c1_mass = f32_infinity;
      if(c1->kind == kind_wall && c0->kind == kind_wall)
	continue;
      vec2 dp = vec2_sub(c0->pos, c1->pos);
      
      float d = vec2_len(dp) - c0->size - c1->size;
      
      if(d < 0){
	vec2 dpn = vec2_normalize(dp);		
	float m1;
	float m0;
	if(c0_mass == f32_infinity){
	  m1 = 1.0;
	  m0 = 0.0;
	}else if(c1_mass == f32_infinity){
	  m0 = 1.0;
	  m1 = 0.0;
	}else{
	  m1 = c0_mass / (c1_mass + c0_mass);
	  m0 = c1_mass / (c1_mass + c0_mass);
	}
	
	c0->pos = vec2_add(c0->pos, vec2_scale(dpn,-1 * m0 * d));
	c1->pos = vec2_add(c1->pos, vec2_scale(dpn, m1 * d));
	dp = vec2_sub(c0->pos, c1->pos);

	vec2 dv = vec2_sub(c0->vel, c1->vel);
	vec2 cp = vec2_add(c0->pos, vec2_scale(dpn, c0->size));

	float vl = vec2_len(dv);
	vec2 dcp0 = vec2_normalize(vec2_sub(c0->pos, cp));
	if(i == 0){
	  logd("%f %f\n", vl, m0);
	}
	c0->vel = vec2_add(c0->vel, vec2_scale(dcp0, -1.5 * m0 * vl));
	
	vec2 dcp1 = vec2_normalize(vec2_sub(c1->pos, cp));
	c1->vel = vec2_add(c1->vel, vec2_scale(dcp1, 1.5 *m1 * vl));
	
      }
    }
  }
  
  
}

void render_game(){
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  circle * circles = persist_alloc("game", sizeof(circle) * 10);
  u64 n_circles = persist_size(circles) / sizeof(circle);
  vec2 offset = circles[0].pos;
  offset.x -= w->width / 2;
  offset.y -= w->height / 2;
  static int initialized = false;
  static int shader = -1;
  if(!initialized){
    char * vs = read_file_to_string("fullscreen_quad.vert");
    char * fs = read_file_to_string("s1.frag");
    shader = load_simple_shader(vs, strlen(vs), fs, strlen(fs));
    logd("Shader: %i\n", shader);
    initialized = true;
  }
  glUseProgram(shader);
  struct{
    float x, y;
    float s;
    float r, g, b;
  }circles2[10];
  vec2 positions[10];
  struct{
    float r, g, b;
  }colors[10];
  float sizes[10];
  glClearColor(0.25,0.25,0.25,1);
  glClear(GL_COLOR_BUFFER_BIT);
  u32 incr = array_count(circles2);
  for(u64 i = 0; i < n_circles; i += incr){
    u32 scnt = MIN(n_circles - i, incr);
    for(u32 j = 0; j < scnt; j++){
      circle c = circles[i + j];
      positions[j] = vec2_sub(c.pos, offset);
      sizes[j] = c.size;
      colors[j].r = c.color.x;
      colors[j].g = c.color.y;
      colors[j].b = c.color.z;
    }
    int colors_loc = glGetUniformLocation(shader, "colors");
    int sizes_loc = glGetUniformLocation(shader, "sizes");
    int positions_loc = glGetUniformLocation(shader, "positions");
    //glUniform1fv(circles_loc, 6 * scnt, (float *) circles2);
    //glUniform1f(colors, circles2[2].x);
    glUniform1i(glGetUniformLocation(shader, "cnt"), scnt);
    glUniform1fv(sizes_loc, 10, sizes);
    glUniform3fv(colors_loc, 10, (float *)colors);
    glUniform2fv(positions_loc, 10, (float *)positions);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
  }
}
