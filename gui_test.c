#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
#include "gui_test.h"
GLFWwindow * windows[10] = {};


window * get_window(GLFWwindow * glwindow){
  window * w = persist_alloc("win_state", sizeof(window));
  for(u32 i = 0; i < array_count(windows); i++){
    if(windows[i] == glwindow)
      return w + i;
  }
  return NULL;
}

void window_pos_callback(GLFWwindow* glwindow, int xpos, int ypos)
{
  window * w = get_window(glwindow);
  w->x = xpos;
  w->y = ypos;
  glfwGetWindowPos(glwindow, &w->x, &w->y);
}

void window_size_callback(GLFWwindow* glwindow, int width, int height)
{
  window * w = get_window(glwindow);
  w->width = width;
  w->height = height;
}

void cursor_pos_callback(GLFWwindow * glwindow, double x, double y){
  window * w = get_window(glwindow);
  controller_state * controller = persist_alloc("controller", sizeof(controller_state));
  x -= w->width * 0.5;
  y -= w->height * 0.5;
  controller->direction = vec2_normalize(vec2_new(x, -y));
  
}

void mouse_button_callback(GLFWwindow * glwindow, int button, int action, int mods){
  UNUSED(glwindow); UNUSED(mods);
  controller_state * controller = persist_alloc("controller", sizeof(controller_state));
  if(action == GLFW_PRESS && button == 0){
    controller->btn1_clicked = true;
    edit_mode * edit = persist_alloc("edit", sizeof(edit_mode));
    edit->left_clicked = true;
  }
}

void scroll_callback(GLFWwindow * glwindow, double x, double y){
  UNUSED(glwindow);UNUSED(x);
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


void load_window(u64 id){
  window * w = persist_alloc("win_state", sizeof(window));
  int cnt = (int)(persist_size(w)  / sizeof(window));
  int index = -1;
  for(int i = 0; i < cnt; i++){
    if(w[i].id == id && w[i].initialized){
      index = i;
      break;
    }else if(w[i].initialized == false)
      index = i;
  }
  if(index == -1){
    w = persist_realloc(w, ( cnt + 1 ) * sizeof(window));
    index = cnt;
    cnt += 1;
  }
  ASSERT(cnt <= 10);
  w = w + index;
  
  if(w->initialized == false){
    w->width = 640;
    w->height = 640;
    w->initialized = true;
    w->id = id;
    sprintf(w->title, "%s", "Win!");
  }
  glfwWindowHint(GLFW_DECORATED, GL_FALSE);
  GLFWwindow * window = glfwCreateWindow(w->width, w->height, w->title, NULL, NULL);
  glfwSetWindowPos(window, w->x, w->y);
  glfwSetWindowSize(window, w->width, w->height);
  windows[index] = window;

  glfwSetWindowPosCallback(window, window_pos_callback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetScrollCallback(window, scroll_callback);
  
}

window * find_window(u64 id){
  window * w = persist_alloc("win_state", sizeof(window));
  u32 cnt = persist_size(w) / sizeof(window);
  for(u32 i = 0; i < MIN(cnt, array_count(windows)); i++){
    if(w[i].id == id)
      return w + i;
  }
  return NULL;
}

GLFWwindow * find_glfw_window(u64 id){
  window * w = persist_alloc("win_state", sizeof(window));
  u32 cnt = persist_size(w) / sizeof(window);
  for(u32 i = 0; i < MIN(cnt, array_count(windows)); i++){
    if(w[i].id == id)
      return windows[i];
  }
  return NULL;
}

window * get_window_glfw(GLFWwindow * win){
  UNUSED(win);
  ASSERT(false);
  return NULL;
}

typedef struct{
  bool active;
  u64 id;
  thickness t;
}thickness_item;


void set_margin(u64 itemid, thickness t){
  thickness_item * w = persist_alloc("item_thickness", sizeof(thickness_item));
  u32 cnt = persist_size(w) / sizeof(thickness_item);
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == itemid && w[i].active){
      w[i].t = t;
      return;
    }
  }
  w = persist_realloc(w, sizeof(thickness_item) * (cnt + 1));
  w[cnt].t = t;
  w[cnt].id = itemid;
  w[cnt].active = true;

}

thickness get_margin(u64 itemid){
  thickness_item * w = persist_alloc("item_thickness", sizeof(thickness_item));
  u32 cnt = persist_size(w) / sizeof(thickness_item);
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == itemid && w[i].active){
      return w[i].t;
    }
  }
  thickness zero = {};
  return zero;
}

void render_window(window * win, bool last){
  thickness margin = get_margin(win->id);
  margin.left += 0.05f;
  GLFWwindow * glfwWin = find_glfw_window(win->id);
  glfwMakeContextCurrent(glfwWin);
  glClearColor(sinf(margin.left * 0.3)* 0.5 + 0.5, sinf(margin.left * 0.33)* 0.5 + 0.5,
	       sinf(margin.left * 0.39)* 0.5 + 0.5, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  set_margin(win->id, margin);
  if(last)
    glfwSwapInterval(1);
  else
    glfwSwapInterval(0);
  glfwSwapBuffers(glfwWin);
}

typedef struct{
  bool active;
  u64 id;
  char name[32 - sizeof(u64) - sizeof(bool)];
}named_item;

u64 get_unique_number(){
  static u64 * w = NULL;
  if(w == NULL){
    w = persist_alloc("get_unique_number", sizeof(u64));
    *w = 10;
  }
  *w += 1;
  return *w;

}
named_item * get_named_item(const char * table, const char * name, bool create){
  named_item * w = persist_alloc(table, sizeof(named_item));
  int cnt = (int)(persist_size(w)  / sizeof(named_item));
  named_item * inactive = NULL;
  for(int i = 0; i < cnt; i++){
    if(w[i].active && 0 == strncmp(name, w[i].name, sizeof(inactive->name))){
      
      inactive = w + i;
   
      break;
    }else if(!w[i].active && inactive == NULL){
      inactive = w + i;
    }
  }
  
  if(!create) return NULL;
  
  
  if(inactive == NULL){
    w = persist_realloc(w, sizeof(named_item) * (cnt + 1));
    inactive = w + cnt;
  }
  inactive->active = true;
  memcpy(inactive->name, name, MIN(strlen(name) + 1, sizeof(inactive->name)));
  return inactive;
}



window * get_window2(const char * name){
  named_item * nw = get_named_item("window_name", name, true);
  if(nw->id == 0)
    nw->id = get_unique_number();
  GLFWwindow * win = find_glfw_window(nw->id);
  logd("Loading window: %i %p %s %s\n", nw->id, win, name, nw->name);
  if(win == NULL)
    load_window(nw->id);
  ASSERT(find_glfw_window(nw->id) != NULL);
  return find_window(nw->id);
}

control_pair * add_control(u64 id, u64 other_id){
  control_pair * w = persist_alloc("control_pairs", sizeof(control_pair));
  u32 cnt = persist_size(w) / sizeof(control_pair);
  control_pair * free = NULL;
  for(u32 i = 0; i < cnt; i++){
    if(w[i].parent_id == id && w[i].child_id == other_id && w[i].active){
      return w + i;
    }else if(w[i].active == false){
      free = w + i;
    }
  }
  if(free == NULL){
    w = persist_realloc(w, sizeof(control_pair) * (cnt + 1));
    free = w + cnt;
  }
  free->active = true;
  free->parent_id = id;
  free->child_id = other_id;
  return free;
}

/*
textline * get_textline(u64 parent_id, const char * name){

}

button * get_button(u64 parent_id, const char * name){

}

stackpanel * get_stackpanel(u64 parent_id, const char * name){

}
*/

rectangle * find_rectangle(u64 id){
  rectangle * w = persist_alloc("rectangles", sizeof(rectangle));
  u32 cnt = persist_size(w) / sizeof(rectangle);
  rectangle * free = NULL;
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == id && w[i].active){
      return w + i;
    }else if(w[i].active == false){
      free = w + i;
    }
  }
  if(free == NULL){
    w = persist_realloc(w, sizeof(rectangle) * (cnt + 1));
    free = w + cnt;
  }
  free->active = true;
  free->id = id;
  return free;
}

rectangle * get_rectangle(const char * name){
  named_item * nw = get_named_item("rectangle_name", name, true);
  if(nw->id == 0)
    nw->id = get_unique_number();
  return find_rectangle(nw->id);
}

void test_gui(){
  named_item * item = get_named_item("window_name", "error_window2", true);
  if(item->id == 0)
    item->id = get_unique_number();
  named_item * item2 = get_named_item("window_name", "error_window", true);
  if(item2->id == 0)
    item2->id = get_unique_number();
  logd("%i %i\n", item->id, item2->id);
  ASSERT(item->id != item2->id);
  
  logd("Got item: '%s'\n", item->name);
  
  window * win = get_window2("error_window");
  logd("window opened: %p\n", win);
  rectangle * rect = get_rectangle("rect1");
  rect->offset = vec2_new(10, 10);
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(1, 0, 0);
  control_pair * pair = add_control(win->id, rect->id);
  logd("pair: %i %i\n", pair->parent_id, pair->child_id);
  

  char buffer[100];
  
  for(int i = 0; i < 6000; i++){
    window * w = persist_alloc("win_state", sizeof(window));

    int cnt = (int)(persist_size(w)  / sizeof(window));
    u64 ts = timestamp();
    for(int j = 0; j < cnt; j++){
      GLFWwindow * win = find_glfw_window(w[j].id);
      if(win == NULL)
	load_window(w[j].id);
      if(w[j].initialized)
	render_window(w + j, j == cnt - 1);
    }
    
    u64 ts2 = timestamp();
    double dt = ((float)(ts2 - ts)) * 1e-6;
    UNUSED(dt);
    logd("dt: %fs\n", dt);
    controller_state * controller = persist_alloc("controller", sizeof(controller_state));
    controller->btn1_clicked = false;
    glfwPollEvents();
    if(controller->btn1_clicked){
      sprintf(buffer, "window%i", get_unique_number());
      get_window2(buffer);
      
    }
  }
  
  /*textline * text = get_text_line(win->id, "error_text");
  button * ok_btn = get_button(win->id, "ok_btn");
  button * cancel_btn = get_button(win->id, "cancel_btn");
  stackpanel * stackpanel = get_stack_panel(win->id, "buttons_panel");
  set_margin(ok_btn->id, 2, 2, 2, 2);
  set_margin(cancel_btn->id, 2, 2, 2, 2);
  set_margin(text->id, 2, 10, 0, 0);
  add_control(stackpanel->id, ok_btn->id);
  add_control(stackpanel->id, cancel_btn->id);
  add_control(win->id, ok_btn->id);
  add_control(win->id, cancel_btn->id);
  add_control(win->id, text->id);
  render_window(win);*/
}
