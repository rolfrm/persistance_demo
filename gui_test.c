#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
#include "gui_test.h"
#include "persist_oop.h"
#include "gui.h"
#include "console.h"
//#include "command.h"

// #Int

vec3 vec3_rand(){
  float rnd(){ return 0.001 * (rand() % 1000); }
  return vec3_new(rnd(), rnd(), rnd());
}


typedef struct{
  u64 id;
}player;

player * get_player(u64 id){
  return find_item("players", id, sizeof(player), true);
}


void rectangle_clicked(u64 control, double x, double y){
  UNUSED(x);UNUSED(y);
  logd("WUUT!\n");
  rectangle * rect = get_rectangle(control);
  ASSERT(rect != NULL);
  if(mouse_button_action == 1)
    rect->color = vec3_new(0, 0, 0);
  else{
    rect->color = vec3_new(rand() % 100, rand() % 100, rand() % 100);
    rect->color = vec3_scale(rect->color, 0.01);
  }
}


u64 render_entity_method;

void render_chunk(chunk * c){
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
   glUniform4f(color_loc, 0, 1, 1, 1.0);
  vec2 size = vec2_new(10, 10);
  vec2 offset = vec2_new(0, 0);
  glUniform2f(offset_loc, offset.x, offset.y);
  glUniform2f(size_loc, size.x, size.y);
  glUniform2f(window_size_loc, window_size.x, window_size.y);
  for(int i = 0; i < 64; i++){
    for(int j = 0; j < 64; j++){
      int idx = j + i * 64;
      u64 id = c->ids[idx];
      if(id == 0) continue; 
      vec2 offset = vec2_mul(vec2_new(i, j), size);
      method render = get_method(id, render_entity_method);
      if(render != NULL)
	render(id, offset);
    }
  }
}

void measure_chunk(u64 control, vec2 * size){
  thickness margin = get_margin(control);
  *size = vec2_new(640 + margin.left + margin.right, 640 + margin.up + margin.down);
}


void render_entity(u64 entity, vec2 offset){
  UNUSED(entity);
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
  vec2 size = vec2_new(10, 10);
  glUniform2f(offset_loc, offset.x, offset.y);
  glUniform2f(size_loc, size.x, size.y);
  glUniform2f(window_size_loc, window_size.x, window_size.y);

  glUniform4f(color_loc, 1, 1, 1, 1);
  glUniform2f(offset_loc, offset.x, offset.y);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);      
   
}


chunk * test_chunk = NULL;
void render_canvas(u64 canvas_id){
  UNUSED(canvas_id);
  player * pl = get_player(intern_string("player_1"));
  if(once(pl->id)){
    render_entity_method = intern_string("render_entity_method");
    define_method(pl->id, render_entity_method, (method) render_entity);
  }
  
  test_chunk = alloc0(sizeof(chunk));
  test_chunk->ids[64 * 32 + 10] = pl->id;
  render_chunk(test_chunk);
}

void render_canvas2(u64 canvas_id){
  UNUSED(canvas_id);
  float rx = 0.0, ry = 0.0, rz = 0.0;
  mat4 rm = mat4_rotate_X(mat4_rotate_Y(mat4_rotate_Z(mat4_identity(), rz), ry), rx);
  mat4 perspective = mat4_perspective(1.0, 1.0, 0.1, 100.0);
  mat4 cam = mat4_mul(perspective, rm);

  static bool initialized = false;
  static int shader = 0;
  if(!initialized){
    char * vs = read_file_to_string("line_shader.vs");
    char * fs = read_file_to_string("line_shader.fs");
    shader = load_simple_shader(vs, strlen(vs), fs, strlen(fs));
    logd("Shader: %i\n", shader);
    dealloc(vs);dealloc(fs);
    initialized = true;
  }
  glViewport(0,0,300,300);
  
  vec3 pts[] = {vec3_new(0,0,0), vec3_new(1,1,1)};
  vec3 endpts[array_count(pts) * 2];
  for(u64 i = 0; i < array_count(pts); i++){
    pts[i * 2] = mat4_mul_vec3(rm, pts[i]);
    pts[i * 2 + 1] = pts[i * 2];
    pts[i * 2] = vec3_scale(pts[i * 2], 1000);
    pts[i * 2 + 1] = vec3_scale(pts[i * 2 + 1], -1000);
  }
  endpts[0] = vec3_new(0,0,0);
  endpts[1] = vec3_new(1,1,1);
  int cam_loc = glGetUniformLocation(shader, "camera");
  uniform_mat4(cam_loc, cam);
  glVertexPointer(3, GL_FLOAT, sizeof(endpts[0]), (float *) endpts);
  glDrawArrays(GL_POINTS, 0, 4);      
  glViewport(0,0, window_size.x, window_size.y);
  
}

CREATE_TABLE(phase, u64, float);

u64 get_unique_number2(){
  static u64 x = 0xFFFF;
  return x++;
}

void create_console(u64 id);
#define UID ({u64 get_number(){static u64 number = 0; if(number == 0) number = get_unique_number2(); return number;}; get_number;})

void test_gui(){
  init_gui();
  window * win = make_window(4);
  sprintf(win->title, "%s", "Test Window");
  logd("window opened: %p\n", win);

  ASSERT(intern_string("game_canvas") != 0);
  
  u64 game_canvas = intern_string("game_canvas");

  define_method(game_canvas, render_control_method, (method) render_canvas);
  define_method(game_canvas, measure_control_method, (method) measure_chunk);
  stackpanel * panel = get_stackpanel(intern_string("stackpanel1"));
  set_horizontal_alignment(panel->id, HALIGN_CENTER);
  panel->orientation = ORIENTATION_VERTICAL;
  set_margin(panel->id, (thickness){30,30,20,20});
  ASSERT(panel->id != win->id && panel->id != 0);
  add_control(win->id, panel->id);
  rectangle * rect = get_rectangle(intern_string("rect1"));
  define_method(rect->id, mouse_down_method, (method) rectangle_clicked);
  if(once(rect->id)){
    rect->size = vec2_new(30, 30);
    rect->color = vec3_new(1, 0, 0);
    set_margin(rect->id, (thickness){1,1,3,3});
    set_corner_roundness(rect->id, (thickness) {1, 1, 1, 1});
  }
  add_control(panel->id, rect->id);
  ASSERT(rect->id != panel->id);

  rect = get_rectangle(intern_string("rect2"));
  if(once(rect->id)){
    rect->size = vec2_new(30, 30);
    rect->color = vec3_new(1, 1, 0);
    set_margin(rect->id, (thickness){1,1,3,3});
    thickness readback = get_margin(rect->id);
    logd("%i: %f %f %f %f\n", rect->id, readback.left, readback.up, readback.right, readback.down);
    ASSERT(get_margin(rect->id).up == 1);
    
    add_control(panel->id, rect->id);
  }
  set_horizontal_alignment(rect->id, HALIGN_RIGHT);
  rect->color = vec3_new(1,0,0);
  rect = get_rectangle(intern_string("rect3"));
  if(once(rect->id)){
    set_margin(rect->id, (thickness){1,1,1,3});
    rect->size = vec2_new(30, 30);
    rect->color = vec3_new(1, 1, 1);
    add_control(panel->id, rect->id);
  }
  
  u64 txtid = get_textline(intern_string("txt1"));
  if(once(txtid)){
    set_text(txtid, "Click me!");
    set_margin(txtid, (thickness){40,3,40,3});
  }
  
  stackpanel * panel2 = get_stackpanel(intern_string("stackpanel_nested"));
  add_control(panel->id, panel2->id);
  add_control(panel2->id, rect->id);

  u64 btn_test = intern_string("button_test");
  if(once(btn_test)){
    set_margin(btn_test, (thickness) {10, 1, 10, 1});
    define_subclass(btn_test, ui_element_class);
  }

  void rectangle_clicked(u64 control, double x, double y){
    rectangle * r = get_rectangle(control);
    vec2 size = r->size;
    if(x > size.x || y > size.y || x < 0 || y < 0) return;
    
    r->color = vec3_rand();
  }
  define_method(rectangle_class, mouse_down_method, (method) rectangle_clicked);
  
  int color_idx = 0;
  vec3 colors[] = {vec3_new(1,0,0), vec3_new(0,1,0), vec3_new(0, 0, 1)};
  
  void button_clicked2(u64 control, double x, double y){
    UNUSED(x);UNUSED(y);UNUSED(control);
    logd("Clicked!\n");
    rectangle * r = get_rectangle(intern_string("rect4"));
    r->color = colors[color_idx];
    color_idx = (color_idx + 1) % array_count(colors);
  }

  void button_render(u64 control){
    u64 index = 0;
    control_pair * child_control = NULL;
    vec2 _size = vec2_zero;
    while((child_control = get_control_pair_parent(control, &index))){
       if(child_control == NULL)
	break;
       rectangle * rect = find_rectangle(child_control->child_id, false);
       if(rect != NULL)
	 continue;
       vec2 size = measure_sub(child_control->child_id);
       _size = vec2_max(_size, size);
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
  
  define_method(btn_test, mouse_down_method, (method) button_clicked2);
  define_method(btn_test, render_control_method, (method) button_render);
  
  rect = get_rectangle(intern_string("rect4"));
  if(once(rect->id)){
    set_margin(rect->id, (thickness){0,0,0,0});
    rect->size = vec2_new(40, 40);
    rect->color = vec3_new(0.5, 0.5, 0.5);
  }
  add_control(btn_test, rect->id);
  add_control(btn_test, txtid);
  add_control(panel2->id, btn_test);

  void color_render_shifter(u64 control){

    rectangle * r = find_rectangle(control, false);
    if(r == NULL) return;
    int _phase = get_phase(control);
    set_phase(control, _phase + 1);
    float phase = _phase * 0.1;
    float cal(float phase){ return sin(phase) * 0.5 + 1.0;}
    r->color = vec3_new(cal(phase), cal(phase * 1.5), cal(phase * 2.0));
    u64 index = 0;
    u64 base = get_baseclass(control, &index);
    method base_method = get_method(base, render_control_method);
    ASSERT(base_method != NULL);
    base_method(control);   
  }

  rect = get_rectangle(intern_string("rect5"));
  if(once(rect->id)){
    set_margin(rect->id, (thickness){3,3,3,3});
    rect->size = vec2_new(100, 30);
    rect->color = vec3_new(1, 1, 1);
  }
  add_control(panel->id, rect->id);
  define_method(rect->id, render_control_method, (method) color_render_shifter);

  rect = get_rectangle(intern_string("rect6"));
  if(once(rect->id)){
    set_margin(rect->id, (thickness){1,1,1,3});
    rect->size = vec2_new(30, 50);
    rect->color = vec3_new(1, 1, 1);
  }
  add_control(panel->id, rect->id);
  
  rect = get_rectangle(intern_string("rect7"));
  if(once(rect->id)){
    set_margin(rect->id, (thickness){6,6,6,6});
    rect->size = vec2_new(50, 30);
    rect->color = vec3_new(1, 0, 1);
  }
  add_control(panel2->id, rect->id);


  u64 console = intern_string("console");
  set_focused_element(win->id, console);
  set_console_height(console, 100);
  create_console(console);
  add_control(win->id, console);
  set_margin(console, (thickness){5,5,5,5});

  void command_entered(u64 id, char * cmd){
    logd("%i (%s)\n", id, cmd);
  }
  
  define_method(console, console_command_entered_method, (method)command_entered);

  void print_console(const char * fmt, va_list lst){
    char buf[200];
    vsprintf(buf, fmt, lst);
    push_console_history(console, buf);
  }
  iron_log_printer = print_console;
  //char buffer[100];
  
  while(true){
    window * w = persist_alloc("win_state", sizeof(window));
    int cnt = (int)(persist_size(w)  / sizeof(window));
    bool any_active = false;
    for(int j = 0; j < cnt; j++){
      if(!w[j].id != 0) continue;
      any_active = true;
      auto method = get_method(w[j].id, render_control_method);
      if(method!= NULL) method(w[j].id);
    }
    if(!any_active){
      logd("Ending program\n");
      break;
    }

    controller_state * controller = persist_alloc("controller", sizeof(controller_state));
    controller->btn1_clicked = false;
    glfwPollEvents();
    if(controller->btn1_clicked){
      //sprintf(buffer, "window%i", get_unique_number());
      //get_window2(buffer);
    }
  }
}
