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
  //glfwWindowHint(GLFW_DECORATED, GL_FALSE);
  static GLFWwindow * ctx = NULL;
  
  GLFWwindow * window = glfwCreateWindow(w->width, w->height, w->title, NULL, ctx);
  if(ctx == NULL){
    ctx = window;
    glfwMakeContextCurrent(window);
    glewInit();
  }
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



typedef struct{
  bool active;
  u64 id;
  char name[32 - sizeof(u64) - sizeof(bool)];
}named_item;

u64 get_unique_number(){
  static u64 * w = NULL;
  if(w == NULL){
    w = persist_alloc("get_unique_number", sizeof(u64));
    if(*w == 0)
      *w = 10;
    //*w = 10;
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
      
      return w + i;
   
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
  inactive->id = 0;

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

control_pair * get_control_pair_parent(u64 parent_id, u64 * index){
  control_pair * w = persist_alloc("control_pairs", sizeof(control_pair));
  u32 cnt = persist_size(w) / sizeof(control_pair);
  for(; *index < cnt; (*index)++){
    if(w[*index].parent_id == parent_id && w[*index].active){
      control_pair * out = w + *index;
      *index += 1;
      return out;
    }
  }
  return NULL;
}

/*
textline * get_textline(u64 parent_id, const char * name){

}

button * get_button(u64 parent_id, const char * name){

}

stackpanel * get_stackpanel(u64 parent_id, const char * name){

}
*/

rectangle * find_rectangle(u64 id, bool create){
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
  if(!create) return NULL;
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
  return find_rectangle(nw->id, true);
}

/*u32 get_rectangle_shader(){
  

  }*/
__thread vec2 shared_offset, shared_size, window_size;

void measure_rectangle(rectangle * rect, vec2 * offset, vec2 * size){
  *offset = vec2_zero;
  vec2 rsize = rect->size;
  *size = vec2_min(rsize, shared_size);
}

void render_rectangle(rectangle * rect){
  UNUSED(rect);
  //logd("rendering rectangle :"); vec2_print(shared_offset); vec2_print(shared_size);logd("\n");
  static int initialized = false;
  static int shader = -1;
  if(!initialized){
    char * vs = read_file_to_string("rect_shader.vs");
    char * fs = read_file_to_string("rect_shader.fs");
    shader = load_simple_shader(vs, strlen(vs), fs, strlen(fs));
    logd("Shader: %i\n", shader);
    initialized = true;
  }
  
  glUseProgram(shader);
  int color_loc = glGetUniformLocation(shader, "color");

  int offset_loc = glGetUniformLocation(shader, "offset");

  int size_loc = glGetUniformLocation(shader, "size");

  int window_size_loc = glGetUniformLocation(shader, "window_size");
  glUniform4f(color_loc, rect->color.x, rect->color.y, rect->color.z, 1.0);
  vec2 size = rect->size;
  vec2 offset = shared_offset;//vec2_add(shared_offset, rect->offset);
  glUniform2f(offset_loc, offset.x, offset.y);
  glUniform2f(size_loc, size.x, size.y);
  glUniform2f(window_size_loc, window_size.x, window_size.y);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

stackpanel * get_stackpanel(const char * name){
  named_item * nw = get_named_item("stackpanel_name", name, true);
  if(nw->id == 0)
    nw->id = get_unique_number();
  return find_stackpanel(nw->id);
}

stackpanel * find_stackpanel(u64 id){
  stackpanel * w = persist_alloc("stackpanels", sizeof(stackpanel));
  u32 cnt = persist_size(w) / sizeof(stackpanel);
  stackpanel * free = NULL;

  for(u32 i = 0; i < cnt; i++){
    
    if(w[i].id == id && w[i].active){
      return w + i;
    }else if(w[i].active == false){
      free = w + i;
    }
  }
  if(free == NULL){
    w = persist_realloc(w, sizeof(stackpanel) * (cnt + 1));
    free = w + cnt;
  }
  free->active = true;
  free->id = id;
  return free;
}



void measure_control(u64 control, vec2 * offset, vec2 * size){
  rectangle * rect = find_rectangle(control, false);
  vec2 soffset = shared_offset;
  thickness margin = get_margin(control);
  shared_offset = vec2_add(soffset, vec2_new(margin.left, margin.up));
  vec2 ssize = shared_size;
  shared_size = vec2_sub(shared_size, soffset);
  if(rect != NULL)
    measure_rectangle(rect, offset, size);
  *size = vec2_add(*size, vec2_new(margin.right, margin.up));
  shared_offset = soffset;
  shared_size = ssize;
  
}

void render_stackpanel(stackpanel * stk){
  u64 index = 0;
  control_pair * child_control = NULL;
  vec2 soffset = shared_offset;
  while((child_control = get_control_pair_parent(stk->id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = vec2_new(0,0);
    vec2 offset = vec2_new(0, 0);
    measure_control(child_control->child_id, &offset, &size);
    rectangle * rect = NULL;
    stackpanel * sstk = NULL; ;
    if((rect = find_rectangle(child_control->child_id, false)) != NULL){
      render_rectangle(rect);
      
    } else if((sstk = find_stackpanel(child_control->child_id)) != NULL){
      render_stackpanel(sstk);
      
    }else{
      child_control->active = false;
    }
    if(stk->orientation == ORIENTATION_HORIZONTAL){
      shared_offset.x += size.x;
    }else{
      shared_offset.y += size.y;
    }
  }
  shared_offset = soffset;
}

void render_window(window * win, bool last){
  thickness margin = get_margin(win->id);
  //margin.left += 0.05f;
  GLFWwindow * glfwWin = find_glfw_window(win->id);

  glfwGetWindowSize(glfwWin, &win->width, &win->height);
  glfwMakeContextCurrent(glfwWin);
  glViewport(0, 0, win->width, win->height);
  window_size = vec2_new(win->width, win->height);
  
  
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  shared_offset = vec2_new(margin.left, margin.up);
  shared_size = vec2_new(win->width - margin.left - margin.right, win->height - margin.up - margin.down);
  u64 index = 0;
  control_pair * child_control = NULL;
  while((child_control = get_control_pair_parent(win->id, &index))){
    if(child_control == NULL)
      break;
    
    thickness margin = get_margin(child_control->child_id);
    rectangle * rect = NULL;
    stackpanel * stk = NULL; ;
    vec2 offset = vec2_new(margin.left,margin.up);
    shared_offset = vec2_add(shared_offset, offset);

    if((rect = find_rectangle(child_control->child_id, false)) != NULL){
      render_rectangle(rect);
    } else if((stk = find_stackpanel(child_control->child_id)) != NULL){
      render_stackpanel(stk);
      
    }else{
      child_control->active = false;

    }
    shared_offset = vec2_sub(shared_offset, offset);
  }
  //set_margin(win->id, margin);
  if(last)
    glfwSwapInterval(1);
  else
    glfwSwapInterval(0);
  glfwSwapBuffers(glfwWin);
}

void test_gui(){
  named_item * item_1 = get_named_item("test_named_item", "hello", true);
  if(item_1->id == 0)
    item_1->id = get_unique_number();
  //return;
  named_item * item_2 = get_named_item("test_named_item", "hello2", true);
  logd("New id: %i\n", item_2->id);
  if(item_2->id == 0)
    item_2->id = get_unique_number();
  ASSERT(item_1->id != item_2->id);
  
  //return;
  
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

  stackpanel * panel = get_stackpanel("stackpanel1");
  panel->orientation = ORIENTATION_VERTICAL;
  set_margin(panel->id, (thickness){300,100,200,200});
  ASSERT(panel->id != win->id && panel->id != 0);
  add_control(win->id, panel->id);
  
  rectangle * rect = get_rectangle("rect1");
  //rect->offset = vec2_new(10, 10);
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(1, 0, 0);
  set_margin(rect->id, (thickness){1,1,3,3});
  control_pair * pair = add_control(panel->id, rect->id);
  ASSERT(rect->id != panel->id);
  logd("%i %i\n", panel->id, rect->id);
  UNUSED(pair);
  rect = get_rectangle("rect2");
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(1, 1, 0);
  set_margin(rect->id, (thickness){1,1,3,3});
  pair = add_control(panel->id, rect->id);
  rect = get_rectangle("rect3");
  set_margin(rect->id, (thickness){1,1,1,3});
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(1, 1, 1);

  pair = add_control(panel->id, rect->id);  
  /*//logd("pair: %i %i %i\n", pair->parent_id, pair->child_id, rect->id);
  rect = get_rectangle("rect2");
  //rect->offset = vec2_new(40, 40);
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(0.6, 0.9, 1);
  set_margin(rect->id, (thickness){36,40,30,30});
  pair = add_control(panel->id, rect->id);  
  logd("pair: %i %i %i\n", pair->parent_id, pair->child_id, rect->id);
  rect = get_rectangle("rect3");
  //  rect->offset = vec2_new(70, 40);
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(0.6, 0.6, 1);
  set_margin(rect->id, (thickness){30,30,30,30});
  pair = add_control(win->id, rect->id);
  rect = get_rectangle("rect4");
  //  rect->offset = vec2_new(70, 70);
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(0.3, 0.3, 1);
  pair = add_control(win->id, rect->id);
  
  rect = get_rectangle("rect5");
  //  rect->offset = vec2_new(70, 100);
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(0.7, 0.3, 0);
  pair = add_control(win->id, rect->id);
  */
  //return;

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
    //logd("dt: %fs\n", dt);
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
