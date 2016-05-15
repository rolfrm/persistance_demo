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

void load_random_level(){
  circle * circles = persist_alloc("game", sizeof(circle) * 40);

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
      if(kind == kind_player){
	circles[i + 1].kind = kind_gun;
	circles[i + 1].size = 3.0;
	circles[i + 1].active = true;
	circles[i + 1].color = vec3_new(1,1,1);
	circles[i + 1].phase = 1.0;
      }
    }
  }
}

circle * _get_new_circle(circle ** circles, u64 * cnt){
  for(u64 i = 0; i < *cnt; i++){
    if(!(*circles)[i].active){
      
      return (*circles) + i;
    }
  }

  *cnt += 1;
  *circles = persist_realloc(*circles, sizeof(circle) * *cnt);
  return &(*circles)[*cnt - 1];
}

circle * get_new_circle(circle ** circles, u64 * cnt){
  circle * c = _get_new_circle(circles, cnt);
  memset(c, 0, sizeof(c[0]));
  return c;
}


void game_loop(){
  //ad_random_level();
  
  map_pos * mpos = persist_alloc("map_pos", sizeof(map_pos));
  controller_state * ctrl = persist_alloc("controller", sizeof(controller_state));

  circle * circles = persist_alloc("game", sizeof(circle));
  //if(circles[0].kind != kind_player || circles[0].active == false ||circles[1].kind != kind_gun){
  //  circles[0].active = false;
  //  load_random_level();
  //}
  
  ASSERT(circles[0].kind == kind_player);
  u64 n_circles = persist_size(circles) / sizeof(circle);
  int active_circles = 0;

  for(u64 i = 0; i < n_circles; i++){
    
    if(!circles[i].active)
      continue;
    
    ASSERT(false == isnan(circles[i].pos.x) && false == isnan(circles[i].pos.y));
    ASSERT(false == isnan(circles[i].vel.x) && false == isnan(circles[i].vel.y));
    ASSERT(isfinite(circles[i].pos.x) && isfinite(circles[i].pos.y));
    ASSERT(isfinite(circles[i].vel.x) && isfinite(circles[i].vel.y));

    
    active_circles += 1;
    if(i == 0)
      circles[i].vel = vec2_add(circles[i].vel, vec2_scale(ctrl->axis.xy, 0.1));
    
    if(circles[i].kind != kind_bullet)
      circles[i].vel = vec2_scale( circles[i].vel, 0.98);
    if(circles[i].kind == kind_bullet){
      circles[i].phase += 1.0;
      if(circles[i].phase > 400)
	circles[i].active = false;
    }
    if(circles[i].kind == kind_gun){
      circle * c = circles + i;
      circle * cp = circles + i - 1;
      c->pos = vec2_add(cp->pos , vec2_new(sinf(c->phase) * 10.0, cosf(c->phase) * 10.0));
      c->vel = cp->vel;
    }
  }
  mpos->offset = circles[0].pos;
  
  circles[1].phase = atan2f(ctrl->direction.x, ctrl->direction.y);
  if(ctrl->btn1_clicked){
    circles[0].color = vec3_new(1.0, 0.6, 0.4);
    circle * bullet = get_new_circle(&circles,&n_circles);
    bullet->vel = vec2_scale(ctrl->direction, 10);
    bullet->pos = circles[1].pos;
    bullet->active = true;
    bullet->size = 2;
    
    bullet->color = vec3_new(1,1,1);
    bullet->phase = 0.0;
    bullet->kind = kind_bullet;
      
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
  
  for(u64 i = 0; i < n_circles; i++){
    circle * c0 = circles + i;
    if(c0->active == false || c0->kind == kind_coin || c0->kind == kind_gun)
      continue;
    f32 c0_mass = c0->size;
    if(c0->kind == kind_wall || c0->kind == kind_turret)
      c0_mass = f32_infinity;
    if(c0->kind == kind_bullet)
      c0_mass *= 0.2;
    for(u64 j = i + 1; j < n_circles; j++){
      circle * c1 = circles + j;
      if(c1->active == false || c1->kind == kind_coin|| c1->kind == kind_gun)
	continue;
      f32 c1_mass = c1->size;
      if(c1->kind == kind_wall  || c1->kind == kind_turret)
	c1_mass = f32_infinity;
      if(c1->kind == kind_wall && c0->kind == kind_wall )
	continue;
      if(c1->kind == kind_bullet)
	c1_mass *= 0.2;
      //if(c1->kind == kind_bullet && c0->kind == kind_bullet)
      //  continue;
      
      vec2 dp = vec2_sub(c0->pos, c1->pos);
      
      float d = vec2_len(dp) - c0->size - c1->size;
      vec2 dv = vec2_sub(c0->vel, c1->vel);
      
      {
	vec2 v = vec2_scale(dv, -1);
	float r = c0->size + c1->size;
	float a = v.x * v.x + v.y * v.y;
	float b = -2 * (v.x * dp.x + v.y * dp.y);
	float c = dp.x * dp.x + dp.y * dp.y - r * r;
	float det = b * b - 4 * a * c;
	float x1 = (-b + sqrtf(det)) / (2.0f * a);
	float x2 = (-b - sqrtf(det)) / (2.0f * a);
	//if(i == 0 && j == 217){
	//  logd("Results %f %f %f\n", det, x1, x2);
	//}
	//if(det >= 0 && )
	//  
	if(det >= 0 && isnan(det) == false && isfinite(det)){
	  if(x1 < 0 || x2 > 1 || x2 < 0){
	  }else{
	    
	    // move them so they touch and resolve collision normally.
	  
	    c0->pos = vec2_add(c0->pos, vec2_scale(c0->vel, x2));
	    c1->pos = vec2_add(c1->pos, vec2_scale(c1->vel, x2));
	    dp = vec2_sub(c0->pos, c1->pos);
	    d = vec2_len(dp) - c0->size - c1->size;
	    vec2_print(v);
	    vec2_print(dp);
	    logd("sweep resolve at t: %f %f %f %f %f %f %f\n", x1, x2, d, det, a, b, c);
	    // resolve collision
	  }
	  
	}
      }

      
      
      if(d <= 0.001){
	if((c1->kind == kind_target && c0->kind == kind_bullet) || (c0->kind == kind_target && c1->kind == kind_bullet)){
	  if(c0->kind == kind_target)
	    c0->active = false;
	  else
	    c1->active = false;
	  continue;
	}
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
	//float d2 = c0->size + c1->size;
	
	//vec2 cp = vec2_add(c0->pos, vec2_scale(dpn, c0->size));

	//float vl = vec2_len(dv);
	
	//vec2 dcp0 = vec2_normalize(vec2_sub(c0->pos, cp));
	float b = vec2_mul_inner(dv, dp) / vec2_sqlen(dp);	
	c0->vel = vec2_sub(c0->vel, vec2_scale(dp, 2 * m0 * b ));

	
	//vec2 dcp1 = vec2_normalize(vec2_sub(c1->pos, cp));
	c1->vel = vec2_sub(c1->vel, vec2_scale(dp, -2 * m1 * b));
	if(i == 0){
	  logd("Collision with %i, size: %f, kind: %i\n", j, c1->size, c1->kind);
	}
	if(c1->kind == kind_bullet)
	  c1->active = false;
	if(c1->kind == kind_bullet && c0->kind == kind_turret){
	  turret * t = find_turret(c0);
	  t->health--;
	  logd("%i \n", t->health);
	  c0->size *= 0.8;
	  if(c0->size <= 5)
	    turret_disable(t);
	}
	
	if(c0->kind == kind_bullet)
	  c0->active = false;
	if(c0->kind == kind_bullet && c1->kind == kind_turret){

	  turret * t = find_turret(c1);
	  t->health--;
	  logd("%i \n", t->health);
	  c1->size *= 0.8;
	  if(c1->size <= 5)	    
	    turret_disable(t);
	}
	
      }
    }
  }
  
  for(u64 i = 0; i < n_circles; i++){
    if(!circles[i].active)
      continue;
    circles[i].pos = vec2_add(circles[i].pos, circles[i].vel);
  }
  {//update turrets
    turret * turrets = persist_alloc("turrets", sizeof(turret));
    u64 turret_cnt = persist_size(turrets) / sizeof(turret);
    for(u64 i = 0; i < turret_cnt; i++){
      if(!turrets[i].active)
	continue;
      turrets[i].cooldown -= 1;
      circle bc = circles[turrets[i].base_circle];
      circle * gun = circles + turrets[i].gun_circle;
      vec2 pv = vec2_sub(vec2_add(circles[0].vel, circles[0].pos), bc.pos);
      float player_dist = vec2_len(pv);
      ASSERT(gun->kind == kind_gun);
      if(player_dist < 400){
	gun->pos = vec2_add(bc.pos, vec2_scale(vec2_normalize(pv), bc.size + 1 + gun->size));
	if(turrets[i].cooldown < 0){
	  
	  circle * bullet = get_new_circle(&circles,&n_circles);
	  bullet->vel = vec2_scale(vec2_normalize(pv), 10);
	  bullet->pos = gun->pos;
	  bullet->active = true;
	  bullet->size = 3;
	  
	  bullet->color = vec3_new(1,0,0);
	  bullet->phase = 0.0;
	  bullet->kind = kind_bullet;
	  turrets[i].cooldown = 50;
	}
      }
    }
  }
  
}

void render_game(){
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  window_state * w = persist_alloc("win_state", sizeof(window_state)); 
  circle * circles = persist_alloc("game", sizeof(circle));
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
  const u32 incr = 50;
  vec2 positions[incr];
  struct{
    float r, g, b;
  }colors[incr];
  float sizes[incr];
  vec2 velocities[incr];
  glClearColor(0.7,0.7,1.0,0);
  glClear(GL_COLOR_BUFFER_BIT);

  u32 total_drawen = 0;
  for(u64 i = 0; i < n_circles; i += incr){

    u32 scnt = MIN(n_circles - i, incr);
    int to_draw = 0;
    for(u32 j = 0; j < scnt && i < n_circles; j++){
      circle c = circles[i + j];

      if(c.active == false || c.size < 1) {
	j--;
	i += 1;
	continue;
      }
      vec2 rel_pos = vec2_sub(c.pos,  offset);
      if(vec2_len(vec2_sub(c.pos, circles[0].pos)) - c.size > 1100){
	j--;
	i += 1;
	continue;
      }
      to_draw += 1;
      positions[j] = rel_pos;
      sizes[j] = c.size;
      velocities[j] = vec2_sub(c.vel, circles[0].vel);
      colors[j].r = c.color.x;
      colors[j].g = c.color.y;
      colors[j].b = c.color.z;
    }
    int colors_loc = glGetUniformLocation(shader, "colors");
    int sizes_loc = glGetUniformLocation(shader, "sizes");
    int positions_loc = glGetUniformLocation(shader, "positions");
    int velocities_loc = glGetUniformLocation(shader, "velocities");
    //glUniform1fv(circles_loc, 6 * scnt, (float *) circles2);
    //glUniform1f(colors, circles2[2].x);
    total_drawen += scnt;
    glUniform1i(glGetUniformLocation(shader, "cnt"), to_draw);
    glUniform1fv(sizes_loc, incr, sizes);
    glUniform3fv(colors_loc, incr, (float *)colors);
    glUniform2fv(positions_loc, incr, (float *)positions);
    glUniform2fv(velocities_loc, incr, (float *)velocities);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); 
  }
  //logd("Total drawen %i\n", total_drawen);
}
