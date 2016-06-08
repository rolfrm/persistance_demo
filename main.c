
#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
#include "hsv.h"
#include "gui_test.h"
vec3 vec3_clamp(vec3 v, float min, float max){
  for(int i = 0; i < 3; i++)
    v.data[i] = v.data[i] < min ? min : v.data[i] > max ? max : v.data[i];
  return v;
}

void edit_loop(){
  circle_kinds kinds[] = {kind_wall, kind_enemy, kind_coin,
			  kind_target, kind_turret, kind_worm,
			  kind_laser};
  
  edit_mode * edit = persist_alloc("edit", sizeof(edit_mode));
  map_pos * mpos = persist_alloc("map_pos", sizeof(map_pos));
  circle * circles = persist_alloc("game", sizeof(circle));
  window * w = persist_alloc("win_state", sizeof(window));
  u64 n_circles = persist_size(circles) / sizeof(circle);
  if(edit->left_clicked){
    vec2 v = vec2_add(mpos->offset, vec2_sub(edit->mouse_pos, vec2_new(w->width * 0.5, w->height * 0.5)));    
    if(edit->picking_color || edit->deleting){
      circle * circ2 = NULL;
      for(u64 i = 0; i < n_circles; i++){
	if(circles[i].active && vec2_len(vec2_sub(circles[i].pos, v)) < circles[i].size){
	  logd("Size: %i %f\n", i, circles[i].size);
	  circ2 = circles + i;
	  break;
	}
      }
      if(circ2 != NULL){
	if(edit->deleting){
	  circ2->active = false;
	  if(circ2->kind == kind_turret){
	    turret * turret = find_turret(circ2);
	    logd("Deleting turret..\n");
	    turret_disable(turret);
	  }
	}
	if(edit->picking_color){
	  edit->color = circ2->color;
	  for(u32 i = 0; i < array_count(kinds); i++){
	    if(kinds[i] == (circle_kinds)circ2->kind){
	      edit->selected_kind = i;
	      break;
	    }
	  }
	  edit->size = circ2->size;
	}
      }
      return;
    }
    
    circle * circ = get_new_circle(&circles, &n_circles);
    logd("New item: %i\n", circ - circles);
    circ->kind = kinds[edit->selected_kind];
    circ->pos = v;
    circ->size = edit->size;
    circ->color = edit->color;
    circ->active = true;
    circ->phase = edit->phase;
    circ->vel = vec2_zero;
    if(circ->kind == kind_turret){
      turret * t = get_new_turret();
      t->health = 10;
      circle * gun = get_new_circle(&circles, &n_circles);
       gun->kind = kind_gun;
      gun->pos = vec2_add(v, vec2_new(circ->size + 1, circ->size + 1));
      gun->size = 3;
      gun->color = vec3_new(0.25, 0.25, 0.25);
      gun->active = true;
      gun->vel = vec2_zero;
      t->base_circle = circ - circles;
      t->gun_circle = gun - circles;
      t->active = true; 
    }

    if(circ->kind == kind_worm){
      worm * worms = persist_alloc("worms", sizeof(worm) * 10);
      u64 worm_cnt = persist_size(worms);
      worm * w = get_new_worm(&worms, &worm_cnt);
      w->active = true;
      w->body_cnt = 6;
      u64 i1 = circ - circles;
      w->bodies[0] = i1;
      circle * circ1 = circ;
      for(u64 i = 1; i < w->body_cnt; i++){
	circle * circ2 = get_new_circle(&circles, &n_circles);
	*circ2 = *circ1;
	circ2->size = circ1->size * 0.7;
	circ2->pos = vec2_sub(circ1->pos, vec2_new(circ1->size + circ2->size, 0));
	circ1 = circ2;
	w->bodies[i] = circ2 - circles;
      }
    }

    if(circ->kind == kind_laser){
      laser * lasers = persist_alloc("lasers", sizeof(laser) * 10);
      u64 laser_cnt = persist_size(lasers);
      laser * l = get_new_laser(&lasers, &laser_cnt);
      l->laser = circ - circles;
      l->active = true;
      l->length = 100;
      l->bullet = -1;
    }
    
    
  }
  
  if(edit->changing_color_h || edit->changing_color_s || edit->changing_color_v){
    vec3 hsv = rgb2hsv(vec3_clamp(edit->color,0,1));
    int dim = 0;
    float mult = 0.4;
    if(edit->changing_color_s){
      dim = 0.1;
      mult = 1;
    }
    else if(edit->changing_color_v){
      mult = 0.1;
      dim = 2;
    }
    hsv.data[dim] += edit->scroll_amount * mult;
    edit->color = hsv2rgb(hsv);
    logd("Color "); vec3_print(edit->color);logd("\n");
  }else if(edit->changing_size){
    edit->size += edit->scroll_amount;
    logd("Size: %f\n", edit->size);

    
  }else if(edit->changing_kind){
    edit->selected_kind += edit->scroll_amount;
    edit->selected_kind %= array_count(kinds);
    logd("Kind: %i\n", edit->selected_kind);
  }else if(edit->changing_phase){
    logd("Phase: %f\n", edit->phase);
    edit->phase += edit->scroll_amount * 0.1;
  }
}


typedef struct
{
  bool active;
  int subsec;
  int next;
  
}rope;

typedef struct{
  bool active;
  u64 content;
}hash_entry;

typedef struct{
  u64 bins;
  i64 size;
  u64 entry_size;
}persist_hash_table;

i64 get_value(persist_hash_table * table, u64 hash, u64 * value, i64 * in_out_idx){
  u64 cell = hash % table->bins;

  hash_entry * entries = ((void *) table) + sizeof(persist_hash_table);
  
  while(*in_out_idx < table->size){
    logd("looking at: %i\n", cell + table->bins * *in_out_idx);
    hash_entry * entry = entries + cell + table->bins * *in_out_idx;
    *in_out_idx += 1;
    if(entry->active){
      *value = entry->content;
      return 0;
    }
    
  }
  return -1;
}


void insert_value(persist_hash_table ** _table, u64 hash, u64 value){
  persist_hash_table * table = *_table;
  hash_entry * entries = ((void *) table) + sizeof(persist_hash_table);
  u64 cell = hash % table->bins;
  hash_entry * entry = NULL;
  for(i64 i = 0; i < table->size; i++){
    u64 idx = cell + table->bins * i;
    
    if(!entries[idx].active){
      entry = entries + idx;
    }
    if(entries[idx].active && entries[idx].content == value)
      return;
  }
  if(entry == NULL){
    *_table = persist_realloc(table, sizeof(persist_hash_table) + (table->size + 1) * (sizeof(hash_entry) * table->bins));
    table = *_table;
    table->size += 1;
    hash_entry * entries = ((void *) table) + sizeof(persist_hash_table);
    u64 idx = cell + table->bins * (table->size - 1);
    entry = entries + idx;
  }
  if(entry != NULL){
    entry->active = true;
    entry->content = value;
  }
}


int main(){
  if (!glfwInit())
    return -1;
  test_gui();
  return 0;
  /*persist_hash_table * ht = persist_alloc("hash_table", sizeof(persist_hash_table));
  ht->bins = 1024 * 1024;
  logd("size: %i %i\n", ht->size, ht->bins);
  insert_value(&ht, 25, 32);
  insert_value(&ht, 25, 31);
  insert_value(&ht, 25, 30);

  u64 val;
  i64 it= 0;
  get_value(ht, 25, &val, &it);
  logd("Value: %i %i\n", val, it);
  get_value(ht, 25, &val, &it);
  logd("Value: %i %i\n", val, it);
  get_value(ht, 25, &val, &it);
  logd("Value: %i %i\n", val, it);
  return 0;
  */
  //circle * circles = persist_alloc("game", sizeof(circle) * 40);
  //circles->active = false;

  for(int i = 0; i < 2; i++){
    load_window(i);
  }
  main_state * main_mode = persist_alloc("main_mode", sizeof(main_state));
  GLFWwindow * glwin = find_glfw_window(1);
  
  glfwMakeContextCurrent(glwin);
  glewInit();
  controller_state * controller = persist_alloc("controller", sizeof(controller_state));
  memset(controller, 0, sizeof(controller_state)); // reset controller on start.
  
  glfwSwapInterval(1);
  while(!controller->should_exit){
    controller->axis.x = 0;
    controller->axis.y = 0;
    
    if(glfwGetKey(glwin, GLFW_KEY_LEFT))
      controller->axis.x -= 1;
    
    if(glfwGetKey(glwin, GLFW_KEY_RIGHT))
      controller->axis.x += 1;
    
    if(glfwGetKey(glwin, GLFW_KEY_UP))
      controller->axis.y += 1;
    
    if(glfwGetKey(glwin, GLFW_KEY_DOWN))
      controller->axis.y -= 1;
    window * w = get_window_glfw(glwin);
    glViewport(0, 0, w->width, w->height);
    double mx,my;
    glfwGetCursorPos(glwin, &mx, &my);
    edit_mode * edit = persist_alloc("edit", sizeof(edit_mode));
    edit->mouse_pos.x = mx;
    edit->mouse_pos.y = w->height - my;
    u64 t1 = timestamp();
    render_game();
    if(main_mode->mode == MODE_GAME){
      game_loop();
    }else{
      edit_loop(glwin);
    }

    glfwSwapBuffers(glwin);
    u64 t2 = timestamp();
    float ds = ((double)(t2 - t1)) * 1e-6;
    UNUSED(ds);
    //logd("main time:%f s\n", ds);
    
    edit->scroll_amount = 0;
    edit->picking_color = glfwGetKey(glwin, GLFW_KEY_P);
    edit->left_clicked = false;
    edit->changing_size = glfwGetKey(glwin, GLFW_KEY_S);
    edit->changing_color_v = glfwGetKey(glwin, GLFW_KEY_I);
    edit->changing_color_h = glfwGetKey(glwin, GLFW_KEY_H);
    edit->changing_color_s = glfwGetKey(glwin, GLFW_KEY_C);
    edit->changing_phase = glfwGetKey(glwin, GLFW_KEY_F);
    edit->changing_kind = glfwGetKey(glwin, GLFW_KEY_K);
    edit->deleting = glfwGetKey(glwin, GLFW_KEY_D);
    controller->btn1_clicked = false;
    glfwPollEvents();
    iron_sleep(0.016);
  }
  glfwTerminate();
  return 0;
}
