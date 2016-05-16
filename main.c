#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
#include "hsv.h"

vec3 vec3_clamp(vec3 v, float min, float max){
  for(int i = 0; i < 3; i++)
    v.data[i] = v.data[i] < min ? min : v.data[i] > max ? max : v.data[i];
  return v;
}

circle * get_new_circle(circle ** circles, u64 * cnt);
typedef struct{
  bool left_clicked;
  bool picking_color;
  vec2 mouse_pos;
  int selected_kind;
  int scroll_amount;
  vec3 color;
  float size;
  bool changing_size;

  bool changing_color_h;
  bool changing_color_s;
  bool changing_color_v;
  bool changing_kind;
  bool deleting;
}edit_mode;

typedef enum{
  MODE_GAME = 0,
  MODE_EDIT
}main_modes;
typedef struct{
  main_modes mode;
}main_state;


void window_pos_callback(GLFWwindow* window, int xpos, int ypos)
{
  UNUSED(window);
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  w->x = xpos;
  w->y = ypos;
  glfwGetWindowPos(window, &w->x, &w->y);
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
  UNUSED(window);
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  w->width = width;
  w->height = height;
}

void cursor_pos_callback(GLFWwindow * window, double x, double y){
  UNUSED(window);
  controller_state * controller = persist_alloc("controller", sizeof(controller_state));
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  x -= w->width * 0.5;
  y -= w->height * 0.5;
  controller->direction = vec2_normalize(vec2_new(x, -y));
  
}

void mouse_button_callback(GLFWwindow * window, int button, int action, int mods){
  UNUSED(window); UNUSED(mods);
  controller_state * controller = persist_alloc("controller", sizeof(controller_state));
  if(action == GLFW_PRESS && button == 0){
    controller->btn1_clicked = true;
    edit_mode * edit = persist_alloc("edit", sizeof(edit_mode));
    edit->left_clicked = true;
  }
}

void scroll_callback(GLFWwindow * window, double x, double y){
  UNUSED(window);UNUSED(x);
  edit_mode * edit = persist_alloc("edit", sizeof(edit_mode));
  main_state * main_mode = persist_alloc("main_mode", sizeof(main_state));
  if(main_mode->mode == MODE_EDIT){
    int scroll = y > 0 ? 1 : -1;
    edit->scroll_amount = scroll;
  }
}


void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods){
  UNUSED(win);UNUSED(key);UNUSED(scancode);UNUSED(action);UNUSED(mods);
  controller_state * controller = persist_alloc("controller", sizeof(controller_state));
  main_state * main_mode = persist_alloc("main_mode", sizeof(main_state));
  if(key == GLFW_KEY_TAB && action == GLFW_PRESS){
    if(main_mode->mode == MODE_EDIT){
      main_mode->mode = MODE_GAME;
    }else{
      main_mode->mode = MODE_EDIT;
    }
  }else if(key == GLFW_KEY_ESCAPE){
    controller->should_exit = true;
  }
}

turret * get_new_turret(){

  turret * turrets = persist_alloc("turrets", sizeof(turret));
  u64 cnt = persist_size(turrets) / sizeof(turret);
  for(u64 i = 0; i < cnt; i++){
    if(!turrets[i].active){
      return turrets + i;
    }
  }

  cnt += 1;
  turrets = persist_realloc(turrets, sizeof(turret) * cnt);
  return &turrets[cnt - 1];
}

turret * find_turret(circle * c){
  circle * circles = persist_alloc("game", sizeof(circle));
  turret * turrets = persist_alloc("turrets", sizeof(turret));
  u64 turret_cnt = persist_size(turrets) / sizeof(turret);
  i32 offset = (i32)(c - circles);
  for(u64 i = 0; i < turret_cnt; i++)
    if(turrets[i].active && turrets[i].base_circle == offset)
      return turrets + i;
  return NULL;
}

void turret_disable(turret * turret){
  circle * circles = persist_alloc("game", sizeof(circle));
  logd("Deleting turret..\n");
  ASSERT(turret != NULL);
  turret->active = false;
  logd(">> %i %i\n", turret->gun_circle, turret->base_circle);
  circles[turret->gun_circle].active = false;
  circles[turret->base_circle].active = false;
}


void edit_loop(){
  circle_kinds kinds[] = {kind_wall, kind_enemy, kind_coin, kind_target, kind_turret};
  edit_mode * edit = persist_alloc("edit", sizeof(edit_mode));
  map_pos * mpos = persist_alloc("map_pos", sizeof(map_pos));
  circle * circles = persist_alloc("game", sizeof(circle));
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  u64 n_circles = persist_size(circles) / sizeof(circle);
  //vec2_print(mpos->offset);logd("\n");
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
    circ->phase = 0.0;
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
  }
}

int main(){
  if (!glfwInit())
    return -1;
  //circle * circles = persist_alloc("game", sizeof(circle) * 40);
  //circles->active = false;
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  main_state * main_mode = persist_alloc("main_mode", sizeof(main_state));
  if(w->initialized == false){
    w->width = 640;
    w->height = 640;
    w->x = 200;
    w->y = 200;
    w->initialized = true;
    sprintf(w->title, "%s", "Win!");
  }
  GLFWwindow * window = glfwCreateWindow(w->width, w->height, w->title, NULL, NULL);
  glfwSetWindowPos(window, w->x, w->y);

  glfwSetWindowPosCallback(window, window_pos_callback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwMakeContextCurrent(window);
  glewInit();
  controller_state * controller = persist_alloc("controller", sizeof(controller_state));
  memset(controller, 0, sizeof(controller_state)); // reset controller on start.
  
  glfwSwapInterval(1);
  while(!controller->should_exit){
    controller->axis.x = 0;
    controller->axis.y = 0;
    
    if(glfwGetKey(window, GLFW_KEY_LEFT)){
      controller->axis.x -= 1;
    }
    if(glfwGetKey(window, GLFW_KEY_RIGHT)){
      controller->axis.x += 1;
    }
    if(glfwGetKey(window, GLFW_KEY_UP)){
      controller->axis.y += 1;
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN)){
      controller->axis.y -= 1;
    }

    glViewport(0, 0, w->width, w->height);
    double mx,my;
    glfwGetCursorPos(window, &mx, &my);
    edit_mode * edit = persist_alloc("edit", sizeof(edit_mode));
    edit->mouse_pos.x = mx;
    edit->mouse_pos.y = w->height - my;
    //u64 t1 = timestamp();
    render_game();
    if(main_mode->mode == MODE_GAME){
      game_loop();
    }else{
      edit_loop(window);
    }

    glfwSwapBuffers(window);
    //u64 t2 = timestamp();
    //logd("main time:%f s\n", ((double)(t2 - t1)) * 1e-6);
    
    edit->scroll_amount = 0;
    edit->picking_color = glfwGetKey(window, GLFW_KEY_P);
    edit->left_clicked = false;
    edit->changing_size = glfwGetKey(window, GLFW_KEY_S);
    edit->changing_color_v = glfwGetKey(window, GLFW_KEY_I);
    edit->changing_color_h = glfwGetKey(window, GLFW_KEY_H);
    edit->changing_color_s = glfwGetKey(window, GLFW_KEY_C);
    edit->changing_kind = glfwGetKey(window, GLFW_KEY_K);
    edit->deleting = glfwGetKey(window, GLFW_KEY_D);
    controller->btn1_clicked = false;
    glfwPollEvents();
    //iron_sleep(0.1);
  }
  glfwTerminate();
  return 0;
}
