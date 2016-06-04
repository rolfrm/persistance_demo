#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
#include "gui_test.h"
//#include "command.h"

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

// #Margin
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

__thread vec2 shared_offset, shared_size, window_size;

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

// #Bare Controls
control * find_control(u64 id, bool create){
  control * w = persist_alloc("controls", sizeof(control));
  u32 cnt = persist_size(w) / sizeof(control);
  control * free = NULL;
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == id && w[i].active){
      return w + i;
    }else if(w[i].active == false){
      free = w + i;
    }
  }
  if(!create) return NULL;
  if(free == NULL){
    w = persist_realloc(w, sizeof(control) * (cnt + 1));
    free = w + cnt;
  }
  free->active = true;
  free->id = id;
  return free;
}

control * get_control(const char * name){
  named_item * nw = get_named_item("control_name", name, true);
  if(nw->id == 0)
    nw->id = get_unique_number();
  return find_control(nw->id, true);
}

control * get_control2(u64 id){
  return find_control(id, true);
}
// #Rectangles

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

bool render_control(u64 control){
  rectangle * rect = NULL;
  stackpanel * sstk = NULL;
  if((rect = find_rectangle(control, false)) != NULL){
    render_rectangle(rect);
    
  } else if((sstk = find_stackpanel(control)) != NULL){
    render_stackpanel(sstk);
  }else{
    return false;
  }
  return true;
}

// #Stackpanel
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
    if(!render_control(child_control->child_id))
      child_control->active = false;
    
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
    
    vec2 offset = vec2_new(margin.left,margin.up);
    shared_offset = vec2_add(shared_offset, offset);

    if(!render_control(child_control->child_id))
      child_control->active = false;
    
    shared_offset = vec2_sub(shared_offset, offset);
  }
  //set_margin(win->id, margin);
  if(last)
    glfwSwapInterval(1);
  else
    glfwSwapInterval(0);
  glfwSwapBuffers(glfwWin);
}

named_item * get_named_item2(const char * table, const char * name){
  named_item * item = get_named_item(table, name, true);
  if(item->id == 0)
    item->id = get_unique_number();
  return item;
}

typedef void (* command_handler)(u64 control, ...);
command_handler * get_command_handler2(u64 control_id, u64 command_id, bool create){
  static command_handler handlers[10] = {};
  static bool inited[10] ={};
  static u64 control_ids[10] = {};
  static u64 command_ids[100] = {};
  for(u64 i = 0; i < array_count(inited); i++){
    if(inited[i] && control_ids[i] == control_id &&  command_ids[i] == command_id)
      return handlers + i;
  }
  if(create){
    for(u64 i = 0; i < array_count(inited); i++){
      if(!inited[i]){
	inited[i] = true;
	control_ids[i] = control_id;
	command_ids[i] = command_id;
	return handlers + i;
      }
    }
  }
  return NULL;
}

void attach_handler(u64 control_id, u64 command_id, void * handler){
  *get_command_handler2(control_id, command_id, true) = handler;
}


command_handler get_command_handler(u64 control_id, u64 command_id){
  command_handler * item = get_command_handler2(control_id, command_id, false);
  return item == NULL ? NULL : *item;
}

typedef struct{
  u64 id;
  bool active;
}class;

class * new_class(u64 id){
  bool create = true;
  class * w = persist_alloc("classes", sizeof(class));
  u32 cnt = persist_size(w) / sizeof(class);
  class * free = NULL;
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == id && w[i].active){
      return w + i;
    }else if(w[i].active == false){
      free = w + i;
    }
  }
  if(!create) return NULL;
  if(free == NULL){
    w = persist_realloc(w, sizeof(class) * (cnt + 1));
    free = w + cnt;
  }
  free->active = true;
  free->id = id;
  return free;
}

u64 intern_string(const char * name){
  return get_named_item2("interned_strings", name)->id;
}

typedef struct{
  bool active;
  u64 base_class;
  u64 class;
}subclass;

subclass * define_subclass(u64 class, u64 base_class){
  subclass * w = persist_alloc("subclasses", sizeof(subclass));
  u32 cnt = persist_size(w) / sizeof(subclass);
  subclass * free = NULL;
  for(u32 i = 0; i < cnt; i++){
    if(w[i].base_class == base_class && w[i].class == class && w[i].active){
      return w + i;
    }else if(w[i].active == false){
      free = w + i;
    }
  }
  if(free == NULL){
    w = persist_realloc(w, sizeof(subclass) * (cnt + 1));
    free = w + cnt;
  }
  free->active = true;
  free->class = class;
  free->base_class = base_class;
  return free;
}

u64 get_baseclass(u64 class, u64 * index){
  subclass * w = persist_alloc("subclasses", sizeof(subclass));
  u64 cnt = persist_size(w) / sizeof(subclass);
  for(; *index < cnt; *index += 1){
    u64 i = *index;
    if(w[i].active && w[i].class == class){
      u64 result = w[i].base_class;
      *index += 1;
      return result;
    }
  }
  return 0;
}

// idea: Dynamic inheritance. An item can inherit from multiple other items.
// technically there is no type. Its all just
// So all buttons inherit from button_class, but button_class is not a type, its just an ID.

void define_method(u64 class_id, u64 method_id, command_handler handler){
  attach_handler(class_id, method_id, handler);
}

command_handler get_method(u64 class_id, u64 method_id){
  command_handler cmd = get_command_handler(class_id, method_id);
  if(cmd == NULL){
    u64 idx = 0;
  next_cls:;
    u64 baseclass = get_baseclass(class_id, &idx);
    
    if(baseclass != 0){
      logd("Found baseclass: %i\n", baseclass);
      command_handler handler = get_method(baseclass, method_id);
      if(handler == NULL)
	goto next_cls;
      return handler;
    }
  }
  return cmd;
}

void on_btn_clicked(u64 control){
  logd("Control: %i\n", control);
}

void on_btn_clicked2(u64 control){
  logd("Control 2: %i\n", control);
  u64 index = 0;
  u64 baseclass = get_baseclass(control, &index);
  logd("Baseclass? %i\n", baseclass);
  get_method(baseclass, intern_string("button_clicked"))(10);
}

void test_gui(){
  class * ui_element_class = new_class(intern_string("element"));
  class * button_class = new_class(intern_string("button"));
  UNUSED(ui_element_class);UNUSED(button_class);
  
  control * button_item = get_control2(intern_string("btn1"));
  subclass * subcls_ptr = persist_alloc("subclasses", sizeof(subclass));
  subclass * subcls = define_subclass(button_item->id, button_class->id);
  UNUSED(subcls);UNUSED(subcls_ptr);
  u64 button_clicked = intern_string("button_clicked");
  define_method(button_class->id, button_clicked, (command_handler)on_btn_clicked);
  define_method(button_item->id, button_clicked, (command_handler)on_btn_clicked2);
  get_method(button_item->id, button_clicked)(button_item->id);
  //logd("method: %i\n", );
  return;
  
  //define_subclass(button_class->id, ui_element_class->id);
  //define_method(button_class->id, button_clicked, on_button_clicked);
  
  
  
  named_item * item_1 = get_named_item2("test_named_item", "hello");
  named_item * item_2 = get_named_item2("test_named_item", "hello2");
  
  ASSERT(item_1->id != item_2->id);
  named_item * cmd1 = get_named_item2("cmd2", "hello");

  void handle_cmd1(u64 control){
    logd("Control: %i\n", control);
  }
  
  attach_handler(cmd1->id, 0, handle_cmd1);
  get_command_handler(cmd1->id, 0)(cmd1->id);
  
  return;
  
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
  set_margin(panel->id, (thickness){30,30,200,200});
  ASSERT(panel->id != win->id && panel->id != 0);
  add_control(win->id, panel->id);
  
  rectangle * rect = get_rectangle("rect1");
  //rect->offset = vec2_new(10, 10);
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(1, 0, 0);
  set_margin(rect->id, (thickness){1,1,3,3});
  rectangle * r0 = rect;
  control_pair * pair = add_control(panel->id, rect->id);
  ASSERT(rect->id != panel->id);
  logd("%i %i\n", panel->id, rect->id);
  UNUSED(pair);
  rect = get_rectangle("rect2");
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(1, 1, 0);
  rectangle * r1 = rect;
  set_margin(rect->id, (thickness){1,1,3,3});
  pair = add_control(panel->id, rect->id);
  rect = get_rectangle("rect3");
  set_margin(rect->id, (thickness){1,1,1,3});
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(1, 1, 1);
  rectangle * r2 = rect;
  pair = add_control(panel->id, rect->id);

  stackpanel * panel2 = get_stackpanel("stackpanel_nested");
  add_control(panel->id, panel2->id);
  add_control(panel2->id, r0->id);
  add_control(panel2->id, r1->id);
  add_control(panel2->id, r2->id);
  
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
    
    for(int j = 0; j < cnt; j++){
      
      GLFWwindow * win = find_glfw_window(w[j].id);
      if(win == NULL)
	load_window(w[j].id);

      if(!w[j].initialized)
	continue;
      //u64 ts = timestamp();
      render_window(w + j, j == cnt - 1);
      // u64 ts2 = timestamp();
      // double dt = ((float)(ts2 - ts)) * 1e-6;
      // UNUSED(dt);
      // logd("dt: %fs\n", dt);
    }
    

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
