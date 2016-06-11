#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
#include "gui_test.h"
#include "persist_oop.h"
//#include "command.h"

// #Int

vec3 vec3_rand(){
  float rnd(){ return 0.001 * (rand() % 1000); }
  return vec3_new(rnd(), rnd(), rnd());
}

typedef struct{
  bool active;
  u64 id;
  int t;
}int_item;

void set_int(const char * table, u64 itemid, int t){
  int_item * w = persist_alloc(table, sizeof(int_item));
  u32 cnt = persist_size(w) / sizeof(int_item);
  int_item * free = NULL;
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == itemid && w[i].active){
      w[i].t = t;
      if(t == 0)
	w[i].active = false;
      return;
    }else if(w[i].active == false && free == NULL)
      free = w + i;
  }
  if(free == NULL){
    w = persist_realloc(w, sizeof(int_item) * (cnt + 1));
    free = w + cnt;
  }
  free->t = t;
  free->id = itemid;
  free->active = true;
}

int get_int(const char * table, u64 itemid){
  int_item * w = persist_alloc(table, sizeof(int_item));
  u32 cnt = persist_size(w) / sizeof(int_item);
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == itemid && w[i].active)
      return w[i].t;
  }
  return 0;
}

// is_mouse_over
void set_is_mouse_over(u64 item_id, bool v){
  set_int("is_mouse_over", item_id, v);
}

bool get_is_mouse_over(u64 item_id){
  return get_int("is_mouse_over", item_id);
}

void is_mouse_over_clear(){
  int_item * w = persist_alloc("is_mouse_over", sizeof(int_item));
  u32 size = persist_size(w);
  memset(w, 0, size);
}


GLFWwindow * windows[10] = {};

u64 window_close_method = 0;
u64 mouse_over_method = 0;
u64 render_control_method = 0;
u64 measure_control_method  = 0;
u64 mouse_down_method = 0;
__thread bool mouse_button_action;
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
  auto on_mouse_over = get_method(w->id, mouse_over_method);
  is_mouse_over_clear();
  if(on_mouse_over != NULL)
    on_mouse_over(w->id, x, w->height - y - 1, 0);
}

void mouse_button_callback(GLFWwindow * glwindow, int button, int action, int mods){
  UNUSED(glwindow); UNUSED(mods);
  mouse_button_action = action == GLFW_PRESS ? 1 : 0;
  if(action == GLFW_REPEAT){
    mouse_button_action = 2;
  }
  if(button == 0){
    window * w = get_window(glwindow);
    auto on_mouse_over = get_method(w->id, mouse_over_method);
    if(on_mouse_over != NULL){
      double x, y;
      glfwGetCursorPos(glwindow, &x, &y);
      is_mouse_over_clear();

      on_mouse_over(w->id, x, w->height - y - 1, mouse_down_method);
    }
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

void window_close_callback(GLFWwindow * glwindow){

  window * w = get_window(glwindow);
  auto close_method = get_method(w->id, window_close_method);
  if(close_method != NULL)
    close_method(w->id);
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
    sprintf(w->title, "%s", "Test Window");
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
  glfwSetWindowCloseCallback(window, window_close_callback);
  
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
  window * _win = find_window(nw->id);
  _win->active = true;
  return _win;
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

// #Color
typedef struct{
  bool active;
  u64 id;
  vec3 t;
}color_item;

void set_color(u64 itemid, vec3 t){
  color_item * w = persist_alloc("item_color", sizeof(color_item));
  u32 cnt = persist_size(w) / sizeof(color_item);
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == itemid && w[i].active){
      w[i].t = t;
      return;
    }
  }
  w = persist_realloc(w, sizeof(color_item) * (cnt + 1));
  w[cnt].t = t;
  w[cnt].id = itemid;
  w[cnt].active = true;
}

vec3 get_color(u64 itemid){
  color_item * w = persist_alloc("item_color", sizeof(color_item));
  u32 cnt = persist_size(w) / sizeof(color_item);
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == itemid && w[i].active){
      return w[i].t;
    }
  }
  vec3 zero = {};
  return zero;
}

u64 get_unique_number(){
  static u64 * w = NULL;
  if(w == NULL){
    w = persist_alloc("get_unique_number", sizeof(u64));
    if(*w == 0)
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
  inactive->id = get_unique_number();

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
  return find_rectangle(nw->id, true);
}

void measure_rectangle(u64 rect_id, vec2 * offset, vec2 * size){
  rectangle * rect = find_rectangle(rect_id, false);
  thickness margin = get_margin(rect_id);
  *offset = vec2_zero;
  vec2 rsize = rect->size;
  *size = vec2_add(rsize, vec2_new(margin.left + margin.right, margin.up + margin.down));
}

void render_rectangle(u64 rect_id){
  rectangle * rect = find_rectangle(rect_id, false);
  ASSERT(rect != NULL);
						      
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
  
  vec3 color = rect->color;
  if(get_is_mouse_over(rect_id)){
    int color_loc = glGetUniformLocation(shader, "color");

    int offset_loc = glGetUniformLocation(shader, "offset");

    int size_loc = glGetUniformLocation(shader, "size");
    
    int window_size_loc = glGetUniformLocation(shader, "window_size");
    glUniform4f(color_loc, 1, 1, 1, 1.0);
    vec2 size = rect->size;
    vec2 offset = shared_offset;//vec2_add(shared_offset, rect->offset);
    glUniform2f(offset_loc, offset.x - 2 , offset.y - 2);
    glUniform2f(size_loc, size.x +4, size.y +4);
    glUniform2f(window_size_loc, window_size.x, window_size.y);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  

  int color_loc = glGetUniformLocation(shader, "color");

  int offset_loc = glGetUniformLocation(shader, "offset");

  int size_loc = glGetUniformLocation(shader, "size");

  int window_size_loc = glGetUniformLocation(shader, "window_size");
  glUniform4f(color_loc, color.x, color.y, color.z, 1.0);
  vec2 size = rect->size;
  vec2 offset = shared_offset;//vec2_add(shared_offset, rect->offset);
  glUniform2f(offset_loc, offset.x, offset.y);
  glUniform2f(size_loc, size.x, size.y);
  glUniform2f(window_size_loc, window_size.x, window_size.y);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void rectangle_mouse_over(u64 control, double x, double y, u64 method){
  thickness margin = get_margin(control);
  x = x - margin.left;
  y = y - margin.up;
  
  rectangle * rect = find_rectangle(control, false);
  ASSERT(rect != NULL);
  if(x < 0 || x > rect->size.x || y < 0 || y > rect->size.y)
    return;
  auto evt = get_method(control, method);
  if(evt != NULL)
    evt(control, x, y);
  set_color(control, vec3_new(rect->color.x * 0.5, rect->color.y * 0.5, rect->color.z * 0.5));
  set_is_mouse_over(control, true);
}

void rectangle_clicked(u64 control, double x, double y){
  UNUSED(x);UNUSED(y);
  logd("WUUT!\n");
  rectangle * rect = find_rectangle(control, false);
  ASSERT(rect != NULL);
  if(mouse_button_action == 1)
    rect->color = vec3_new(0, 0, 0);
  else{
    rect->color = vec3_new(rand() % 100, rand() % 100, rand() % 100);
    rect->color = vec3_scale(rect->color, 0.01);
  }
}

// # Text line
textline * find_textline(u64 id, bool create){
  textline * w = persist_alloc("textlines", sizeof(textline));
  u32 cnt = persist_size(w) / sizeof(textline);
  textline * free = NULL;
  for(u32 i = 0; i < cnt; i++){
    if(w[i].id == id && w[i].active){
      return w + i;
    }else if(w[i].active == false){
      free = w + i;
    }
  }
  if(!create) return NULL;
  if(free == NULL){
    w = persist_realloc(w, sizeof(textline) * (cnt + 1));
    free = w + cnt;
  }
  free->active = true;
  free->id = id;
  return free;
}

textline * get_textline(const char * name){
  named_item * nw = get_named_item("textline_name", name, true);
  return find_textline(nw->id, true);
}

#include "stb_truetype.h"
static bool font_initialized = false;
static stbtt_fontinfo font;
static unsigned char temp_bitmap[1024*1024];
static stbtt_bakedchar cdata[96];
static u32 ftex;
void ensure_font_initialized(){
  if(font_initialized) return;
  font_initialized = true;
  const char * fontfile = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
  u64 buffersize;
  void * buffer = read_file_to_buffer(fontfile, &buffersize);
  stbtt_InitFont(&font, buffer, 0);

  stbtt_BakeFontBitmap(buffer,0, 18.0, temp_bitmap, 1024,1024, 32,96, cdata);
  glGenTextures(1, &ftex);
  glBindTexture(GL_TEXTURE_2D, ftex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024,1024, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, temp_bitmap);
  // can free temp_bitmap at this point
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void measure_textline(u64 control, vec2 * offset, vec2 * size){
  UNUSED(offset);
  ensure_font_initialized();

  textline * txt = find_textline(control, false);
  ASSERT(txt != NULL);

  float x = 0;
  float y = 0;
  for(u64 i = 0; i < array_count(txt->text); i++){
    if(txt->text[i] == 0) break;
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(cdata, 1024, 1024, txt->text[i]-32, &x,&y,&q,1);
  }
  thickness margin = get_margin(control);
  size->x = x + margin.left + margin.right;
  size->y = 15 + margin.up + margin.down;
}


void render_textline(u64 control){

  ensure_font_initialized();

  textline * txt = find_textline(control, false);
  ASSERT(txt != NULL);
  				      
  static int initialized = false;
  static int shader = -1;
  if(!initialized){
    char * vs = read_file_to_string("text_shader.vs");
    char * fs = read_file_to_string("text_shader.fs");
    shader = load_simple_shader(vs, strlen(vs), fs, strlen(fs));
    initialized = true;
  }
  glUseProgram(shader);

  glBindTexture(GL_TEXTURE_2D, ftex);
  int color_loc = glGetUniformLocation(shader, "color");
  int offset_loc = glGetUniformLocation(shader, "offset");
  int size_loc = glGetUniformLocation(shader, "size");
  int window_size_loc = glGetUniformLocation(shader, "window_size");
  int uv_offset_loc = glGetUniformLocation(shader, "uv_offset");
  int uv_size_loc = glGetUniformLocation(shader, "uv_size");
  
  glUniform4f(color_loc, 0, 0, 0, 1.0);

  glUniform2f(window_size_loc, window_size.x, window_size.y);
  float x = 0;
  float y = 0;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  thickness margin = get_margin(control);
  shared_offset.x += margin.left;
  shared_offset.y -= margin.up;
  for(u64 i = 0; i < array_count(txt->text); i++){
    if(txt->text[i] == 0)break;
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(cdata, 1024, 1024, txt->text[i]-32, &x,&y,&q,1);
    
    vec2 size = vec2_new(q.x1 - q.x0, q.y1 - q.y0);
    vec2 offset = vec2_new(q.x0 + shared_offset.x, shared_offset.y - q.y1 + 15);
    glUniform2f(offset_loc, offset.x, offset.y);
    glUniform2f(size_loc, size.x, size.y);
    glUniform2f(uv_offset_loc, q.s0, q.t0);
    glUniform2f(uv_size_loc, q.s1 - q.s0, q.t1 - q.t0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  glDisable(GL_BLEND);
}
// # Button

u64 get_button(u64 id){
  return id;
}



void measure_control(u64 control, vec2 * offset, vec2 * size){

  UNUSED(offset);
  vec2 _size = vec2_zero; 
  measure_child_controls(control, &_size);
  thickness margin = get_margin(control);
  *size = vec2_add(_size, vec2_new(margin.right + margin.left, margin.up + margin.down));
}

void render_control(u64 stk_id){
  control * stk = find_control(stk_id, false);
  u64 index = 0;
  control_pair * child_control = NULL;
  vec2 soffset = shared_offset;
  while((child_control = get_control_pair_parent(stk->id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = vec2_new(0,0);
    vec2 offset = vec2_new(0, 0);
    
    auto measure = get_method(child_control->child_id, measure_control_method);
    if(measure != NULL)measure(child_control->child_id, &offset, &size);
    auto render = get_method(child_control->child_id, render_control_method);
    if(render != NULL)
      render(child_control->child_id);
  }
  shared_offset = soffset;
}


// #Stackpanel
stackpanel * get_stackpanel(const char * name){
  named_item * nw = get_named_item("stackpanel_name", name, true);
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

void stack_panel_mouse_over(u64 stk_id, double x, double y, u64 method){
  stackpanel * stk = find_stackpanel(stk_id);
  u64 index = 0;
  control_pair * child_control = NULL;

  thickness margin = get_margin(stk->id);
  x = x - margin.left;
  y = y - margin.up;
  if(x < 0 || y < 0)
    return;
  auto evt = get_method(stk_id, method);
  if(evt != NULL)
    evt(stk_id, x, y);
  
  while((child_control = get_control_pair_parent(stk->id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = vec2_new(0,0);
    vec2 offset = vec2_new(0, 0);

    auto measure = get_method(child_control->child_id, measure_control_method);
    if(measure != NULL)
      measure(child_control->child_id, &offset, &size);

    auto on_mouse_over = get_method(child_control->child_id, mouse_over_method);
    if(on_mouse_over != NULL)
      on_mouse_over(child_control->child_id, x, y, method);
    
    if(stk->orientation == ORIENTATION_HORIZONTAL){
      x -= size.x;
    }else{
      y -= size.y;
    }
  }
}


void render_stackpanel(u64 stk_id){
  stackpanel * stk = find_stackpanel(stk_id);
  u64 index = 0;
  control_pair * child_control = NULL;
  vec2 soffset = shared_offset;
  while((child_control = get_control_pair_parent(stk->id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = vec2_new(0,0);
    vec2 offset = vec2_new(0, 0);
    
    auto measure = get_method(child_control->child_id, measure_control_method);
    if(measure != NULL)measure(child_control->child_id, &offset, &size);
    auto render = get_method(child_control->child_id, render_control_method);
    if(render != NULL)
      render(child_control->child_id);
    
    if(stk->orientation == ORIENTATION_HORIZONTAL){
      shared_offset.x += size.x;
    }else{
      shared_offset.y += size.y;
    }
  }
  shared_offset = soffset;
}

void measure_stackpanel(u64 stk_id, vec2 * offset, vec2 * s){
  UNUSED(offset);
  thickness margin = get_margin(stk_id);
  vec2 _size = vec2_new(margin.left + margin.right, margin.up + margin.down);
  
  stackpanel * stk = find_stackpanel(stk_id);
  u64 index = 0;
  control_pair * child_control = NULL;
  float max_dim2 = 0.0;
  while((child_control = get_control_pair_parent(stk->id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = vec2_new(0,0);
    vec2 offset = vec2_new(0, 0);
    
    auto measure = get_method(child_control->child_id, measure_control_method);
    if(measure != NULL)
      measure(child_control->child_id, &offset, &size);
    
    
    if(stk->orientation == ORIENTATION_HORIZONTAL){
      _size.x += size.x;
      max_dim2 = MAX(size.y, max_dim2);
    }else{
      _size.y += size.y;
      max_dim2 = MAX(size.x, max_dim2);
    }
  }
  if(stk->orientation == ORIENTATION_HORIZONTAL){
    _size.y += max_dim2;
  }else{
    _size.x += max_dim2;
  }
  *s = _size;
     
}


void render_window(u64 window_id){
  window * win = find_window(window_id);
  {
    GLFWwindow * win = find_glfw_window(window_id);
    if(win == NULL)
      load_window(window_id);
  }
  
  bool last = true;
  thickness margin = get_margin(win->id);
  //margin.left += 0.05f;
  GLFWwindow * glfwWin = find_glfw_window(win->id);
  glfwSetWindowTitle(glfwWin, win->title);
  glfwGetWindowSize(glfwWin, &win->width, &win->height);
  glfwMakeContextCurrent(glfwWin);
  glViewport(0, 0, win->width, win->height);
  window_size = vec2_new(win->width, win->height);
  
  
  glClearColor(0.8, 0.8, 1, 1);
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

    get_method(child_control->child_id, render_control_method)(child_control->child_id);
    
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


u64 intern_string(const char * name){
  return get_named_item2("interned_strings", name)->id;
}

// idea: Dynamic inheritance. An item can inherit from multiple other items.
// technically there is no type. Its all just
// So all buttons inherit from button_class, but button_class is not a type, its just an ID.

void on_btn_clicked(u64 control){
  logd("Control: %i\n", control);
}

void on_btn_clicked2(u64 control){
  logd("Control 2: %i\n", control);
  u64 index = 0;
  u64 baseclass = get_baseclass(control, &index);
  get_method(baseclass, intern_string("button_clicked"))(10);
}

void window_close(u64 win_id){
  UNUSED(win_id);
  
  window * win = find_window(win_id);
  win->active = false;
}

void measure_child_controls(u64 control, vec2 * size){
  u64 index = 0;
  control_pair * child_control = NULL;
  while((child_control = get_control_pair_parent(control, &index))){
    auto measure = get_method(child_control->child_id, measure_control_method);
    if(measure == NULL)
      continue;
    vec2 offset, _size = vec2_zero;
    measure(child_control->child_id, &offset, &_size);
    size->x = MAX(size->x, _size.x);
    size->y = MAX(size->y, _size.y);
  }
}

void ui_element_mouse_over(u64 control, double x, double y, u64 method){
  thickness margin = get_margin(control);
  x = x - margin.left;
  y = y - margin.up;
  auto evt = get_method(control, method);
  if(evt != NULL)
    evt(control, x, y);
  u64 index = 0;
  control_pair * child_control = NULL;
  while((child_control = get_control_pair_parent(control, &index))){
    if(child_control == NULL)
      break;
    auto measure = get_method(child_control->child_id, measure_control_method);
    if(measure == NULL) continue;
    vec2 offset, size = vec2_zero;
    measure(child_control->child_id, &offset, &size);
  
    if(x < size.x && y < size.y){
      
      auto on_mouse_over = get_method(child_control->child_id, mouse_over_method);
      if(on_mouse_over == NULL) continue;
      on_mouse_over(child_control->child_id, x, y, method);
    }
  }
}


void test_gui(){

  class * ui_element_class = new_class(intern_string("ui_element_class"));
  class * button_class = new_class(intern_string("button_class"));
  class * window_class = new_class(intern_string("window_class"));
  class * stack_panel_class = new_class(intern_string("stack_panel_class"));
  class * rectangle_class = new_class(intern_string("rectangle_class"));
  class * textline_class = new_class(intern_string("textline_class"));
  render_control_method = intern_string("render_control");
  measure_control_method = intern_string("measure_control");
  window_close_method = intern_string("window_close");
  mouse_over_method = intern_string("mouse_over");
  mouse_down_method = intern_string("mouse_down");
  
  define_method(ui_element_class->id, render_control_method,
		(method)render_control);
  define_method(window_class->id, render_control_method,
		(method)render_window);
  define_method(stack_panel_class->id, render_control_method,
		(method) render_stackpanel);
  define_method(rectangle_class->id, render_control_method,
		(method) render_rectangle);
  define_method(textline_class->id, render_control_method, (method) render_textline);
  

  define_method(ui_element_class->id, measure_control_method,
		(method) measure_control);
  define_method(rectangle_class->id, measure_control_method,
		(method) measure_rectangle);
  define_method(stack_panel_class->id, measure_control_method,
		(method) measure_stackpanel);
  define_method(textline_class->id, measure_control_method, (method)measure_textline);

  define_method(window_class->id, window_close_method,
		(method) window_close);

  define_subclass(stack_panel_class->id, ui_element_class->id);
  define_subclass(window_class->id, ui_element_class->id);
  control * button_item = get_control2(intern_string("myButton"));
  define_subclass(button_item->id, button_class->id);
  define_method(button_item->id, intern_string("button_clicked"),
		(method)on_btn_clicked);

  define_method(ui_element_class->id, mouse_over_method,
		(method) ui_element_mouse_over);
  define_method(stack_panel_class->id, mouse_over_method,
		(method) stack_panel_mouse_over);
  define_method(rectangle_class->id, mouse_over_method, (method) rectangle_mouse_over);
  
  window * win = get_window2("error_window");
  sprintf(win->title, "%s", "Test Window");
  define_subclass(win->id, window_class->id);
  logd("window opened: %p\n", win);

  stackpanel * panel = get_stackpanel("stackpanel1");
  define_subclass(panel->id, stack_panel_class->id);
  panel->orientation = ORIENTATION_VERTICAL;
  set_margin(panel->id, (thickness){30,30,200,200});
  ASSERT(panel->id != win->id && panel->id != 0);
  add_control(win->id, panel->id);
  
  rectangle * rect = get_rectangle("rect1");
  define_method(rect->id, mouse_down_method, (method) rectangle_clicked);
  define_subclass(rect->id, rectangle_class->id);
  //rect->offset = vec2_new(10, 10);
  rect->size = vec2_new(30, 30);
  rect->color = vec3_new(1, 0, 0);
  set_margin(rect->id, (thickness){1,1,3,3});
  add_control(panel->id, rect->id);
  ASSERT(rect->id != panel->id);
  logd("%i %i\n", panel->id, rect->id);

  rect = get_rectangle("rect2");
  define_subclass(rect->id, rectangle_class->id);
  //rect->size = vec2_new(30, 30);
  //rect->color = vec3_new(1, 1, 0);
  set_margin(rect->id, (thickness){1,1,3,3});
  add_control(panel->id, rect->id);
  
  rect = get_rectangle("rect3");
  define_subclass(rect->id, rectangle_class->id);
  set_margin(rect->id, (thickness){1,1,1,3});
  //rect->size = vec2_new(30, 30);
  //rect->color = vec3_new(1, 1, 1);
  add_control(panel->id, rect->id);

  textline * txt = get_textline("txt1");
  sprintf(txt->text, "Click me!");
  set_margin(txt->id, (thickness){40,3,40,3});
  define_subclass(txt->id, textline_class->id);
  
  
  stackpanel * panel2 = get_stackpanel("stackpanel_nested");
  add_control(panel->id, panel2->id);
  //add_control(panel2->id, r0->id);
  //add_control(panel2->id, r1->id);
  //add_control(panel2->id, txt->id);
  //add_control(panel2->id, r2->id);
  define_subclass(panel2->id, stack_panel_class->id);

  control * btn_test = get_control2(intern_string("button_test"));
  set_margin(btn_test->id, (thickness) {1, 1, 1, 1});
  define_subclass(btn_test->id, ui_element_class->id);

  void rectangle_clicked(u64 control, double x, double y){
    rectangle * r = find_rectangle(control, false);
    vec2 size = r->size;
    if(x > size.x || y > size.y || x < 0 || y < 0) return;
    
    r->color = vec3_rand();
  }
  define_method(rectangle_class->id, mouse_down_method, (method) rectangle_clicked);
  
  int color_idx = 0;
  vec3 colors[] = {vec3_new(1,0,0), vec3_new(0,1,0), vec3_new(0, 0, 1)};
  
  void button_clicked2(u64 control, double x, double y){
    vec2 size;
    measure_child_controls(control, &size);
    if(x > size.x || y > size.y || x < 0 || y < 0) return;
    rectangle * r = get_rectangle("rect4");
    r->color = colors[color_idx];
    color_idx = (color_idx + 1) % array_count(colors);
  }

  void button_render(u64 control){
    u64 index = 0;
    control_pair * child_control = NULL;
    vec2 _size;
    while((child_control = get_control_pair_parent(control, &index))){
       if(child_control == NULL)
	break;
      
      auto measure = get_method(child_control->child_id, measure_control_method);
      if(measure == NULL) continue;
      vec2 size = vec2_new(0,0);
      vec2 offset = vec2_new(0, 0);
      measure(child_control->child_id, &offset, &size);
      _size.x = MAX(_size.x, size.x);
      _size.y = MAX(_size.y, size.y);
    }
    index = 0;
    while((child_control = get_control_pair_parent(control, &index))){
      if(child_control == NULL)
	break;
      rectangle * rect = find_rectangle(child_control->child_id, false);
      if(rect == NULL) continue;
      rect->size = _size;
    }
    index = 0;
    u64 base = get_baseclass(control, &index);
    method base_method = get_method(base, render_control_method);
    ASSERT(base_method != NULL);
    base_method(control);
      
  }
  
  define_method(btn_test->id, mouse_down_method, (method) button_clicked2);
  define_method(btn_test->id, render_control_method, (method) button_render);
  
  rect = get_rectangle("rect4");
  define_subclass(rect->id, rectangle_class->id);
  set_margin(rect->id, (thickness){0,0,0,0});
  rect->size = vec2_new(40, 40);
  rect->color = vec3_new(0.5, 0.5, 0.5);
  add_control(btn_test->id, rect->id);
  add_control(btn_test->id, txt->id);
  add_control(panel2->id, btn_test->id);



  rect = get_rectangle("rect5");
  define_subclass(rect->id, rectangle_class->id);
  set_margin(rect->id, (thickness){3,3,3,3});
  rect->size = vec2_new(10, 30);
  rect->color = vec3_new(1, 1, 1);
  add_control(panel->id, rect->id);

  rect = get_rectangle("rect6");
  define_subclass(rect->id, rectangle_class->id);
  set_margin(rect->id, (thickness){1,1,1,3});
  rect->size = vec2_new(30, 50);
  rect->color = vec3_new(1, 1, 1);
  add_control(panel->id, rect->id);
  
  rect = get_rectangle("rect7");
  define_subclass(rect->id, rectangle_class->id);
  set_margin(rect->id, (thickness){6,6,6,6});
  rect->size = vec2_new(50, 30);
  rect->color = vec3_new(1, 0, 1);
  add_control(panel2->id, rect->id);

  char buffer[100];
  
  for(int i = 0; i < 6000; i++){
    window * w = persist_alloc("win_state", sizeof(window));
    int cnt = (int)(persist_size(w)  / sizeof(window));
    bool any_active = false;
    for(int j = 0; j < cnt; j++){
      if(!w[j].active) continue;
      any_active = true;
      get_method(w[j].id, render_control_method)(w[j].id);
    }
    if(!any_active){
      logd("Ending program\n");
      break;
    }

    controller_state * controller = persist_alloc("controller", sizeof(controller_state));
    controller->btn1_clicked = false;
    glfwPollEvents();
    if(controller->btn1_clicked){
      sprintf(buffer, "window%i", get_unique_number());
      get_window2(buffer);
    }
  }
}
