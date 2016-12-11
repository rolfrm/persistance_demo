#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
#include "sortable.h"
#include "persist_oop.h"

#include "gui.h"

GLFWwindow * find_glfw_window(u64 id);

CREATE_TABLE(once,u64,u8);

bool once(u64 itemid){
  int v = get_once(itemid);
  if(v == 0){
    set_once(itemid, 1);
    return true;
  }
  return false;
}

CREATE_TABLE(is_mouse_over, u64, bool);
CREATE_TABLE(focused_element, u64, u64);
CREATE_TABLE2(window_state, u64, window);

struct{
  GLFWwindow ** glfw_window;
  u64 * window_id;
  u64 cnt;
}windows;


u64 window_close_method = 0;
u64 mouse_over_method = 0;
u64 render_control_method = 0;
u64 measure_control_method  = 0;
u64 mouse_down_method = 0;
u64 char_handler_method = 0;
u64 key_handler_method = 0;

u64 ui_element_class;
u64 button_class;
u64 window_class;
u64 stack_panel_class;
u64 rectangle_class;
u64 textline_class;
__thread bool mouse_button_action;

u64 get_window(GLFWwindow * glwindow){  
  for(u32 i = 0; i < windows.cnt; i++){
    if(windows.glfw_window[i] == glwindow)
      return windows.window_id[i];
  }
  return 0;
}

window * get_window_ref(GLFWwindow * glwindow){
  return get_ref_window_state(get_window(glwindow));
}

void window_pos_callback(GLFWwindow* glwindow, int xpos, int ypos)
{
  window * w = get_window_ref(glwindow);
  w->x = xpos;
  w->y = ypos;
  glfwGetWindowPos(glwindow, &w->x, &w->y);

}

void window_size_callback(GLFWwindow* glwindow, int width, int height)
{  
  window * w = get_window_ref(glwindow);
  w->width = width;
  w->height = height;
}

void cursor_pos_callback(GLFWwindow * glwindow, double x, double y){
  u64 win_id = get_window(glwindow);
  window * win = get_ref_window_state(win_id);
  auto on_mouse_over = get_method(win_id, mouse_over_method);
  clear_is_mouse_over();
  
  GLFWwindow * glfwWin = find_glfw_window(win_id);
  glfwGetWindowSize(glfwWin, &win->width, &win->height);
  thickness margin = get_margin(win_id);
  window_size = vec2_new(win->width, win->height);
  shared_offset = vec2_new(margin.left, margin.up);
  shared_size = vec2_new(win->width - margin.left - margin.right, win->height - margin.up - margin.down);
  if(on_mouse_over != NULL)
    on_mouse_over(win_id, x, y, 0);
}

void mouse_button_callback(GLFWwindow * glwindow, int button, int action, int mods){
  UNUSED(mods);
  mouse_button_action = action == GLFW_PRESS ? 1 : 0;
  if(action == GLFW_REPEAT){
    mouse_button_action = 2;
  }
  if(button == 0){
    u64 win_id = get_window(glwindow);
    window * win = get_window_ref(glwindow);
    auto on_mouse_over = get_method(win_id, mouse_over_method);
    if(on_mouse_over != NULL){
      double x, y;
      glfwGetCursorPos(glwindow, &x, &y);
      clear_is_mouse_over();
      thickness margin = get_margin(win_id);
      //margin.left += 0.05f;
      GLFWwindow * glfwWin = find_glfw_window(win_id);
      glfwGetWindowSize(glfwWin, &win->width, &win->height);
      window_size = vec2_new(win->width, win->height);
      shared_offset = vec2_new(margin.left, margin.up);
      shared_size = vec2_new(win->width - margin.left - margin.right, win->height - margin.up - margin.down);
      on_mouse_over(win_id, x, y, mouse_down_method);
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


void key_callback(GLFWwindow* glwindow, int key, int scancode, int action, int mods){
  UNUSED(scancode);
  u64 win_id = get_window(glwindow);
  u64 focused = get_focused_element(win_id);
  if(focused == 0) return;
  method m = get_method(focused, key_handler_method);
  m(focused, key, mods, action);
}

void char_callback(GLFWwindow * glwindow, u32 codepoint){
  if(0 == codepoint_to_utf8(codepoint,NULL, 10))
    return; // WTF! Invalid codepoint!
  u64 win_id = get_window(glwindow);
  u64 focused = get_focused_element(win_id);
  if(focused == 0) return;
  
  method m = get_method(focused, char_handler_method);
  m(focused, codepoint, 0);
    
}

void window_close_callback(GLFWwindow * glwindow){
  u64 win_id = get_window(glwindow);
  auto close_method = get_method(win_id, window_close_method);
  if(close_method != NULL)
    close_method(win_id);
}


void load_window(u64 id){
  
  window w;
  if(try_get_window_state(id, &w) == false){
    w = (window){.width = 640, .height = 640};
    sprintf(w.title, "%s", "Test Window");
  }
  if(w.height <= 0) w.height = 200;
  if(w.width <= 0) w.width = 200;
  static GLFWwindow * ctx = NULL;
  logd("Window size:  %s %i %i\n", w.title, w.width, w.height);
  GLFWwindow * window = glfwCreateWindow(w.width, w.height, w.title, NULL, ctx);
  ASSERT(window != NULL);
  if(ctx == NULL){
    ctx = window;
    glfwMakeContextCurrent(window);
    glewInit();
  }
  glfwSetWindowPos(window, w.x, w.y);
  glfwSetWindowSize(window, w.width, w.height);
  list_push(windows.window_id, windows.cnt, id);
  list_push2(windows.glfw_window, windows.cnt, window);

  glfwSetWindowPosCallback(window, window_pos_callback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetWindowCloseCallback(window, window_close_callback);
  glfwSetCharCallback(window, char_callback);
  insert_window_state(&id, &w, 1);
}


GLFWwindow * find_glfw_window(u64 id){
  
  for(u32 i = 0; i < windows.cnt; i++){
    if(windows.window_id[i] == id)
      return windows.glfw_window[i];
  }
  return NULL;
}

void make_window(u64 id){
  if(id == 0) id = get_unique_number();
  GLFWwindow * win = find_glfw_window(id);
  if(win == NULL)
    load_window(id);
  ASSERT(find_glfw_window(id) != NULL);
  define_subclass(id, window_class);
}
CREATE_TABLE(margin, u64, thickness);
CREATE_TABLE(corner_roundness, u64, thickness);
CREATE_TABLE_DECL2(color, u64, vec3);
CREATE_TABLE2(color, u64, vec3);
CREATE_TABLE(vertical_alignment, u64, vertical_alignment);
CREATE_TABLE(horizontal_alignment, u64, horizontal_alignment);

__thread vec2 shared_offset, shared_size, window_size;
vec2 measure_sub(u64 item);

int debug_depth = 0;

void handle_mouse_over(u64 id, double x, double y, u64 method){
  auto on_mouse_over = get_method(id, mouse_over_method);
  vertical_alignment va = get_vertical_alignment(id);
  horizontal_alignment ha = get_horizontal_alignment(id);
  
  vec2 cs = measure_sub(id);
  auto margin = get_margin(id);
  vec2 s = vec2_new(MAX(0, shared_size.x - cs.x - margin.left - margin.right),
		    MAX(0, shared_size.y - cs.y - margin.up - margin.down));
  float x2 = 0.;
  if(ha == HALIGN_CENTER)
    x2 = s.x * 0.5;
  else if(ha == HALIGN_RIGHT)
    x2 = s.x;
  float y2 = 0.;
  if(va == VALIGN_CENTER)
    y2 = s.y * 0.5;
  if(va == VALIGN_BOTTOM)
    y2 = s.y;
  vec2 saved_offset = shared_offset;
  vec2 saved_size = shared_size;
  shared_offset.x += x2 + margin.left;
  shared_offset.y += y2 + margin.up;
  shared_size.x = shared_size.x - x2 - margin.right;
  shared_size.y = shared_size.y - y2 - margin.down;
  if(x > shared_offset.x  && y > shared_offset.y && x < shared_offset.x + cs.x && y < shared_offset.y + cs.y && on_mouse_over != NULL)
    on_mouse_over(id, x, y, method);  
  shared_offset = saved_offset;
  shared_size = saved_size;
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

u64 intern_string(const char * name){
  ASSERT(strlen(name) < 56);
  static persisted_mem_area * mem = NULL;
  if(mem == NULL)
    mem = create_mem_area("interned");
  
  size_t cnt = mem->size  / sizeof(named_item);
  named_item * inactive = NULL;
  named_item * w = mem->ptr;
  for(size_t i = 0; i < cnt; i++){
    if(w[i].id != 0 && 0 == strncmp(name, w[i].name, sizeof(inactive->name))){
      return (w + i)->id;
   
    }else if(w[i].id == 0 && inactive == NULL){
      inactive = w + i;
    }
  }
  
  if(inactive == NULL){
    mem_area_realloc(mem, sizeof(named_item) * (cnt + 1));
    w = mem->ptr;
    inactive = w + cnt;
  }
  memcpy(inactive->name, name, MIN(strlen(name) + 1, sizeof(inactive->name)));
  inactive->id = get_unique_number();

  return inactive->id;
}


named_item unintern_string(u64 id){
  static persisted_mem_area * mem = NULL;
  if(mem == NULL)
    mem = create_mem_area("interned");
  
  size_t cnt = mem->size  / sizeof(named_item);
  named_item * w = mem->ptr;
  for(size_t i = 0; i < cnt; i++){
    if(w[i].id == id)
      return w[i];
  }
  return (named_item){};
}




vec2 measure_sub(u64 item){
  auto measure = get_method(item, measure_control_method);
  vec2 s = {};
  if(measure != NULL)
    measure(item, &s);
  thickness margin = get_margin(item);
  s.x += margin.left + margin.right;
  s.y += margin.up + margin.down;
  return s;
}

void debug_render(vec3 color){
  rect_render(color, shared_offset, vec2_new(shared_size.x, 2));
  rect_render(color, shared_offset, vec2_new(2, shared_size.y));
  rect_render(color, vec2_add(shared_offset, vec2_new(0, shared_size.y - 2)), vec2_new(shared_size.x, 2));
  rect_render(color, vec2_add(shared_offset, vec2_new(shared_size.x - 2, 0)), vec2_new(2, shared_size.y));
}

vec3 vec3_rand();
void render_sub(u64 id){
  auto render = get_method(id, render_control_method);
  if(render == NULL)
    return;
  vertical_alignment va = get_vertical_alignment(id);
  horizontal_alignment ha = get_horizontal_alignment(id);
  
  vec2 cs = measure_sub(id);
  auto margin = get_margin(id);
  vec2 s = vec2_new(MAX(0, shared_size.x - cs.x - margin.left - margin.right),
		    MAX(0, shared_size.y - cs.y - margin.up - margin.down));
  float x2 = 0.;
  if(ha == HALIGN_CENTER)
    x2 = s.x * 0.5;
  else if(ha == HALIGN_RIGHT)
    x2 = s.x;
  float y2 = 0.;
  if(va == VALIGN_CENTER)
    y2 = s.y * 0.5;
  if(va == VALIGN_BOTTOM)
    y2 = s.y;
  vec2 saved_offset = shared_offset;
  vec2 saved_size = shared_size;
  //debug_render(vec3_new(1,0,0));
  shared_offset.x += x2 + margin.left;
  shared_offset.y += y2 + margin.up;
  shared_size.x = shared_size.x - x2 - margin.right;
  shared_size.y = shared_size.y - y2 - margin.down;
  //debug_render(vec3_new(1,0.5,0.5));
  
  render(id);
  shared_offset = saved_offset;
  shared_size = saved_size;


}


control_pair * add_control(u64 id, u64 other_id){
  ASSERT(other_id != 0);
  size_t total_size = 0;
  control_pair * w = persist_alloc2("control_pairs", sizeof(control_pair), &total_size);
  u32 cnt = total_size / sizeof(control_pair);
  control_pair * free = NULL;
  for(u32 i = 0; i < cnt; i++){
    if(w[i].parent_id == id && w[i].child_id == other_id){
      return w + i;
    }else if(w[i].parent_id == 0 && free != NULL){
      free = w + i;
    }
  }
  if(free == NULL){
    w = persist_realloc(w, sizeof(control_pair) * (cnt + 1));
    free = w + cnt;
  }
  free->parent_id = id;
  free->child_id = other_id;
  return free;
}

control_pair * get_control_pair_parent(u64 parent_id, u64 * index){
  control_pair * w = persist_alloc("control_pairs", sizeof(control_pair));
  u32 cnt = persist_size(w) / sizeof(control_pair);
  for(; *index < cnt; (*index)++){
    if(w[*index].parent_id == parent_id){
      control_pair * out = w + *index;
      *index += 1;
      return out;
    }
  }
  return NULL;
}

// #Rectangles

rectangle * find_rectangle(u64 id, bool create){

  rectangle * r = find_item("rectangles", id, sizeof(rectangle), create);
  if(create)
    define_subclass(id, rectangle_class);
  return r;
}


rectangle * get_rectangle(u64 id){
  return find_rectangle(id, true);
}

void measure_rectangle(u64 rect_id, vec2 * size){
  rectangle * rect = find_rectangle(rect_id, false);
  if(rect != NULL)
    *size =rect->size;
  
}
void rect_render(vec3 color, vec2 offset, vec2 size){
  rect_render2(color, offset, size, 0, vec2_zero, vec2_zero);
}
void rect_render2(vec3 color, vec2 offset, vec2 size, i32 tex, vec2 uv_offset, vec2 uv_size){
		
  static int initialized = false;
  static int shader = -1;
  static int color_loc;
  static int offset_loc;
  static int size_loc;
  static int window_size_loc;
  //static int tex_loc;
  static int mode_loc;
  static int uv_offset_loc, uv_size_loc;
  if(!initialized){
    char * vs = read_file_to_string("rect_shader.vs");
    char * fs = read_file_to_string("rect_shader.fs");
    shader = load_simple_shader(vs, strlen(vs), fs, strlen(fs));
    dealloc(vs);
    dealloc(fs);
    logd("Shader: %i\n", shader);
    initialized = true;
    color_loc = glGetUniformLocation(shader, "color");
    offset_loc = glGetUniformLocation(shader, "offset");
    size_loc = glGetUniformLocation(shader, "size");
    window_size_loc = glGetUniformLocation(shader, "window_size");
    //tex_loc = glGetUniformLocation(shader, "tex");
    mode_loc = glGetUniformLocation(shader, "mode");
    uv_offset_loc = glGetUniformLocation(shader, "uv_offset");
    uv_size_loc = glGetUniformLocation(shader, "uv_size");
  }
  glUseProgram(shader);
  if(tex != 0){
    glUniform1i(mode_loc, 1);
    glUniform2f(uv_offset_loc, uv_offset.x, uv_offset.y);
    glUniform2f(uv_size_loc, uv_size.x, uv_size.y);
    //glUniform1i(tex_loc, 0);
    glBindTexture(GL_TEXTURE_2D, tex);
  }
  else
    glUniform1i(mode_loc, 0);

  glUniform4f(color_loc, color.x, color.y, color.z, 1.0);
  glUniform2f(offset_loc, offset.x, offset.y);
  glUniform2f(size_loc, size.x, size.y);
  glUniform2f(window_size_loc, window_size.x, window_size.y);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/*
void rect_render3(vec3 * colors, vec2 * offset, float size,
		  vec2 * uv_offset, vec2 * uv_size, float point_size, i32 tex, u64 cnt){
static int initialized = false;
  static int shader = -1;
  static int color_loc;
  static int offset_loc;
  static int size_loc;
  static int window_size_loc;
  //static int tex_loc;
  static int mode_loc;
  static int uv_offset_loc, uv_size_loc;
  if(!initialized){
    char * vs = read_file_to_string("rect_shader2.vs");
    char * fs = read_file_to_string("rect_shader.fs");
    shader = load_simple_shader(vs, strlen(vs), fs, strlen(fs));
    initialized = true;
    offset_loc = glGetUniformLocation(shader, );
    glBindAttribLocation(shader, 0, "offset");
    glBindAttribLocation(shader, 1, "uv_size");
    glBindAttribLocation(shader, 2, "uv_offset");
    glBindAttribLocation(shader, 3, "color");
    window_size_loc = glGetUniformLocation(shader, "window_size");
    //tex_loc = glGetUniformLocation(shader, "tex");
    mode_loc = glGetUniformLocation(shader, "mode");
  }
  glUseProgram(shader);
  if(tex != 0){
    glUniform1i(mode_loc, 1);
    glUniform2f(uv_offset_loc, uv_offset.x, uv_offset.y);
    glUniform2f(uv_size_loc, uv_size.x, uv_size.y);
    //glUniform1i(tex_loc, 0);
    glBindTexture(GL_TEXTURE_2D, tex);
  }
  else
    glUniform1i(mode_loc, 0);
  glPointSize(size);
  
  glUniform4f(color_loc, color.x, color.y, color.z, 1.0);
  glUniform2f(offset_loc, offset.x, offset.y);
  glUniform2f(size_loc, size.x, size.y);
  glUniform2f(window_size_loc, window_size.x, window_size.y);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
  

}*/


void render_rectangle(u64 rect_id){

  rectangle * rect = find_rectangle(rect_id, false);
  ASSERT(rect != NULL);

  vec3 color = rect->color;
  vec2 size = rect->size;
  vec2 offset = shared_offset;
  if(get_is_mouse_over(rect_id))
    rect_render(vec3_new(1,1,1), vec2_new(offset.x - 2, offset.y - 2), vec2_new(size.x + 4, size.y + 4));
  rect_render(color, offset, size);
}

void rectangle_mouse_over(u64 control, double x, double y, u64 method){
  rectangle * rect = find_rectangle(control, false);
  ASSERT(rect != NULL);
  auto evt = get_method(control, method);
  if(evt != NULL)
    evt(control, x, y);
  set_color(control, vec3_new(rect->color.x * 0.5, rect->color.y * 0.5, rect->color.z * 0.5));
  set_is_mouse_over(control, true);
}


// # Text line
CREATE_STRING_TABLE(text, u64);

u64 find_textline(u64 id, bool create){
  if(create)
    define_subclass(id, textline_class);
  return id;
}

u64 get_textline(u64 id){
  return find_textline(id, true);
}

#include "stb_truetype.h"
static bool font_initialized = false;
static unsigned char temp_bitmap[1024*1024];
#define CHAR_DATA_SIZE 300
static stbtt_bakedchar cdata[CHAR_DATA_SIZE];
static u32 ftex;
void ensure_font_initialized(){
  if(font_initialized) return;
  font_initialized = true;
  const char * fontfile = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
  u64 buffersize;
  void * buffer = read_file_to_buffer(fontfile, &buffersize);

  stbtt_BakeFontBitmap(buffer,0, 12.0, temp_bitmap, 1024,1024, 32,CHAR_DATA_SIZE, cdata);
  glGenTextures(1, &ftex);
  glBindTexture(GL_TEXTURE_2D, ftex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024,1024, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, temp_bitmap);
  // can free temp_bitmap at this point
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

vec2 measure_text(const char * text, size_t len){
  ensure_font_initialized();
  float x = 0;
  float y = 0;
  for(u64 i = 0; i < len; i++){
    if(text[i] == 0) break;
    size_t l = 0;
    int codepoint = utf8_to_codepoint(text + i, &l);
    if(codepoint >= 32 && codepoint < (32 + CHAR_DATA_SIZE)){
      stbtt_aligned_quad q;
      stbtt_GetBakedQuad(cdata, 1024, 1024, codepoint-32, &x,&y,&q,1);
    }
  }
  return vec2_new(x, 15);
}

void render_text(const char * text, size_t len){
  ensure_font_initialized();
  
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
  for(u64 i = 0; i < len;){
    if(text[i] == 0) break;
    size_t l = 0;
    int codepoint = utf8_to_codepoint(text + i, &l);
    if(codepoint >= 32 && codepoint < (32 + CHAR_DATA_SIZE)){
      
      stbtt_aligned_quad q;
      stbtt_GetBakedQuad(cdata, 1024, 1024, codepoint-32, &x,&y,&q,1);
    
      vec2 size = vec2_new(q.x1 - q.x0, q.y1 - q.y0);
      vec2 offset = vec2_new(q.x0 + shared_offset.x, shared_offset.y + q.y0 + 15);
      glUniform2f(offset_loc, offset.x, offset.y);
      glUniform2f(size_loc, size.x, size.y);
      glUniform2f(uv_offset_loc, q.s0, q.t0);
      glUniform2f(uv_size_loc, q.s1 - q.s0, q.t1 - q.t0);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    i += l;
  }
  glDisable(GL_BLEND);
}

void measure_textline(u64 control, vec2 * size){
  ensure_font_initialized();
  u64 len = get_text(control, NULL, 1000);
  char text[len];
  get_text(control, text, len);
  *size = measure_text(text, len);
}


void render_textline(u64 control){

  u64 len = get_text(control, NULL, 1000);
  if(len == 0)
    return;
  char text[len];
  get_text(control, text, len);
  render_text(text, len);
}

void measure_control(u64 control, vec2 * size){
  measure_child_controls(control, size);
}

void render_control(u64 stk_id){
  u64 index = 0;
  control_pair * child_control = NULL;
  vec2 soffset = shared_offset;
  while((child_control = get_control_pair_parent(stk_id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = vec2_new(0,0);
    
    auto measure = get_method(child_control->child_id, measure_control_method);
    if(measure != NULL)measure(child_control->child_id, &size);
    auto render = get_method(child_control->child_id, render_control_method);
    if(render != NULL)
      render(child_control->child_id);
  }
  shared_offset = soffset;
}


// #Stackpanel
stackpanel * get_stackpanel(u64 id){
  return find_stackpanel(id);
}

stackpanel * find_stackpanel(u64 id){
  stackpanel * s = find_item("stackpanels", id, sizeof(stackpanel), true);
  define_subclass(s->id, stack_panel_class);
  return s;
}


void stack_panel_mouse_over(u64 stk_id, double x, double y, u64 method){
    stackpanel * stk = find_stackpanel(stk_id);
  u64 index = 0;
  control_pair * child_control = NULL;
  while((child_control = get_control_pair_parent(stk->id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = measure_sub(child_control->child_id);
    vec2 saved_size = shared_size;
    if(stk->orientation == ORIENTATION_HORIZONTAL)
      shared_size.x = MAX(shared_size.x, size.x);
    else
      shared_size.y = MAX(shared_size.y, size.y);
    handle_mouse_over(child_control->child_id, x, y, method);
    shared_size = saved_size;
    if(stk->orientation == ORIENTATION_HORIZONTAL){
      shared_offset.x += size.x;
      shared_size.x -= size.x;
    }else{
      shared_offset.y += size.y;
      shared_size.y -= size.y;
    }
  }
}


void render_stackpanel(u64 stk_id){
  stackpanel * stk = find_stackpanel(stk_id);
  u64 index = 0;
  control_pair * child_control = NULL;
  while((child_control = get_control_pair_parent(stk->id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = measure_sub(child_control->child_id);
    vec2 saved_size = shared_size;
    if(stk->orientation == ORIENTATION_HORIZONTAL)
      shared_size.x = MAX(shared_size.x, size.x);
    else
      shared_size.y = MAX(shared_size.y, size.y);
    render_sub(child_control->child_id);
    shared_size = saved_size;
    if(stk->orientation == ORIENTATION_HORIZONTAL){
      shared_offset.x += size.x;
      shared_size.x -= size.x;
    }else{
      shared_offset.y += size.y;
      shared_size.y -= size.y;
    }
  }
}


void measure_stackpanel(u64 stk_id, vec2 * s){
  vec2 _size = vec2_zero;
  
  stackpanel * stk = find_stackpanel(stk_id);
  u64 index = 0;
  control_pair * child_control = NULL;
  float max_dim2 = 0.0;
  while((child_control = get_control_pair_parent(stk->id, &index))){
    if(child_control == NULL)
      break;
    vec2 size = measure_sub(child_control->child_id);
    
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
  window win = get_window_state(window_id);
  {
    GLFWwindow * win = find_glfw_window(window_id);
    if(win == NULL)
      load_window(window_id);
  }
  
  bool last = true;
  thickness margin = get_margin(window_id);
  //margin.left += 0.05f;
  GLFWwindow * glfwWin = find_glfw_window(window_id);
  glfwSetWindowTitle(glfwWin, win.title);
  glfwGetWindowSize(glfwWin, &win.width, &win.height);
  glfwMakeContextCurrent(glfwWin);
  glViewport(0, 0, win.width, win.height);
  window_size = vec2_new(win.width, win.height);
  
  vec3 color = get_color(window_id);
  glClearColor(color.x, color.y, color.z, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  shared_offset = vec2_new(margin.left, margin.up);
  shared_size = vec2_new(win.width - margin.left - margin.right, win.height - margin.up - margin.down);
  u64 index = 0;
  control_pair * child_control = NULL;
  while((child_control = get_control_pair_parent(window_id, &index))){
    if(child_control == NULL)
      break;
    ASSERT(child_control->child_id != 0);
  
    render_sub(child_control->child_id);
  }
  if(last)
    glfwSwapInterval(1);
  else
    glfwSwapInterval(0);
  glfwSwapBuffers(glfwWin);
}

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
  window * win = get_ref_window_state(win_id);
  UNUSED(win);
}

void measure_child_controls(u64 control, vec2 * size){
  u64 index = 0;
  control_pair * child_control = NULL;
  while((child_control = get_control_pair_parent(control, &index))){
    vec2 _size = measure_sub(child_control->child_id);
    size->x = MAX(size->x, _size.x);
    size->y = MAX(size->y, _size.y);
  }
}

void ui_element_mouse_over(u64 control, double x, double y, u64 method){

  auto evt = get_method(control, method);
  if(evt != NULL)
    evt(control, x, y);
  
  u64 index = 0;
  control_pair * child_control = NULL;
  while((child_control = get_control_pair_parent(control, &index))){
    if(child_control == NULL)
      break;
    handle_mouse_over(child_control->child_id, x, y, method);
  }
}


void init_gui(){
  ui_element_class = new_class(intern_string("ui_element_class"))->id;
  button_class = new_class(intern_string("button_class"))->id;
  window_class = new_class(intern_string("window_class"))->id;
  stack_panel_class = new_class(intern_string("stack_panel_class"))->id;
  rectangle_class = new_class(intern_string("rectangle_class"))->id;
  textline_class = new_class(intern_string("textline_class"))->id;  

  render_control_method = intern_string("render_control");
  measure_control_method = intern_string("measure_control");
  window_close_method = intern_string("window_close");
  mouse_over_method = intern_string("mouse_over");
  mouse_down_method = intern_string("mouse_down");
  char_handler_method = intern_string("handle_char");
  key_handler_method = intern_string("handle_key");
  
  define_method(ui_element_class, render_control_method, (method)render_control);
  define_method(window_class, render_control_method, (method)render_window);
  define_method(stack_panel_class, render_control_method, (method) render_stackpanel);
  define_method(rectangle_class, render_control_method, (method) render_rectangle);
  define_method(textline_class, render_control_method, (method) render_textline);

  define_method(ui_element_class, measure_control_method, (method) measure_control);
  define_method(rectangle_class, measure_control_method, (method) measure_rectangle);
  define_method(stack_panel_class, measure_control_method, (method) measure_stackpanel);
  define_method(textline_class, measure_control_method, (method)measure_textline);
  define_method(window_class, window_close_method, (method) window_close);

  define_subclass(stack_panel_class, ui_element_class);
  define_subclass(window_class, ui_element_class);

  define_method(ui_element_class, mouse_over_method, (method) ui_element_mouse_over);
  define_method(stack_panel_class, mouse_over_method, (method) stack_panel_mouse_over);
  define_method(rectangle_class, mouse_over_method, (method) rectangle_mouse_over);
}

int key_backspace = 259;
int key_enter = 257;
int key_space = 32;
int key_release = 0;
int key_press = 1;
int key_repeat = 2;
int mod_ctrl= 2;


