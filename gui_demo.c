#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
//#include "gui_test.h"
#include "sortable.h"
#include "persist_oop.h"

#include "gui.h"
#include "console.h"

#include "animation.h"
#include "command.h"
#include "index_table.h"
#include <ecl/ecl.h>

char * ecl_string_to_c_heap(cl_object val){
  bool basestr = ECL_BASE_STRING_P(val);
  bool str = ECL_STRINGP(val);
  if(str){
    auto s = val->string;
    int len = 0;
    for(cl_index i = 0; i < s.dim; i++){
      len += codepoint_to_utf8(s.self[i], NULL, len + 8);
    }

    char * data = GC_MALLOC(len + 1);
    memset(data, 0, len + 1);
    int idx = 0;
    for(cl_index i = 0; i < s.dim; i++){
      idx += codepoint_to_utf8(s.self[i], data + idx, len - idx);
    }
    return data;
  }else if(basestr){
    auto s = val->base_string;
    UNUSED(s);
    ERROR("Base string type not supported.");
    
  }else{
    ERROR("Not a string type");
  }
  return NULL;
}

cl_object cfun_test(cl_object val){
    logd("--- TEST FCN CALLED! ---");
    return val;
  }

cl_object clgui_string_id(cl_object name){
  auto namestr = ecl_string_to_c_heap(name);
  u64 nameid = intern_string(namestr);
  return ecl_make_fixnum(nameid);
}

static index_table * sortable_table = NULL;


void register_sortable(sorttable * table){
  if(sortable_table == NULL){
    sortable_table = index_table_create(NULL, sizeof(table));
  }

  auto table_idx = index_table_alloc(sortable_table);
  ((sorttable **)index_table_lookup(sortable_table, table_idx))[0] = table;
}

cl_object clgui_get_sortable_count(){
  if(sortable_table == NULL)
    return ecl_make_fixnum(0);
  return ecl_make_fixnum(index_table_count(sortable_table) - 1);
}
#include <ctype.h>

cl_object clgui_get_sortable_data(cl_object ptr){
  auto fixnum = ecl_to_fixnum(ptr) + 1;
  sorttable ** _table = index_table_lookup(sortable_table, fixnum);
  auto table = _table[0];
  char buf[strlen(table->name) + 1];
  for(u32 i = 0; i < sizeof(buf) - 1; i++){
    buf[i] = toupper(table->name[i]);
  }
  buf[sizeof(buf) - 1] = 0;
  
  return cl_list(5,ecl_make_keyword(buf), c_string_to_object(table->key_type),
		 c_string_to_object(table->value_type), ecl_make_fixnum(table->key_size),
		 ecl_make_fixnum(table->value_size));
}

cl_object float_to_cl_object(float v){
  return ecl_make_single_float(v);
}

float cl_object_to_float(cl_object obj){
  switch(ecl_t_of(obj)){
  case t_fixnum:
    return (float) ecl_fixnum(obj);
  case t_singlefloat:
    return obj->SF.SFVAL;
  case t_doublefloat:
    return (float)obj->DF.DFVAL;
  case t_longfloat:
    return (float)obj->longfloat.value;
  default:
    ASSERT(false); 
  }
  return 0;
}

i64 cl_object_to_i64(cl_object obj){
  switch(ecl_t_of(obj)){
  case t_fixnum:
    return (i64) ecl_fixnum(obj);
  default:
    ASSERT(false); 
  }
  return 0;
}

cl_object double_to_cl_object(double v){
  return ecl_make_double_float(v);
}

double cl_object_to_double(cl_object obj){
  switch(ecl_t_of(obj)){
  case t_fixnum:
    return (double) ecl_fixnum(obj);
  case t_singlefloat:
    return (double)obj->SF.SFVAL;
  case t_doublefloat:
    return obj->DF.DFVAL;
  case t_longfloat:
    return (double)obj->longfloat.value;
  default:
    ASSERT(false); 
  }
  return 0;
}

vec2 cl_object_to_vec2(cl_object obj){
  cl_object x = cl_car(obj);
  cl_object y = cl_cadr(obj);
  return vec2_new(cl_object_to_float(x), cl_object_to_float(y));
}

cl_object vec2_to_cl_object(vec2 v){
  return cl_list(2, float_to_cl_object(v.x), float_to_cl_object(v.y));
}


vec3 cl_object_to_vec3(cl_object obj){
  cl_object x = cl_car(obj);
  cl_object y = cl_cadr(obj);
  cl_object z = cl_caddr(obj);
  return vec3_new(cl_object_to_float(x), cl_object_to_float(y), cl_object_to_float(z));
}

cl_object vec3_to_cl_object(vec3 v){
  return cl_list(3, float_to_cl_object(v.x), float_to_cl_object(v.y), float_to_cl_object(v.z));
}

vec4 cl_object_to_vec4(cl_object obj){
  cl_object x = cl_car(obj);
  cl_object y = cl_cadr(obj);
  cl_object z = cl_caddr(obj);
  cl_object w = cl_cadddr(obj);
  return vec4_new(cl_object_to_float(x), cl_object_to_float(y), cl_object_to_float(z), cl_object_to_float(w));
}

cl_object vec4_to_cl_object(vec4 v){
  return cl_list(4, float_to_cl_object(v.x), float_to_cl_object(v.y), float_to_cl_object(v.z), float_to_cl_object(v.w));
}



cl_object clgui_set_property(cl_object sortable, cl_object name, cl_object value){
  auto fixnum = ecl_to_fixnum(sortable) + 1;
  sorttable ** _table = index_table_lookup(sortable_table, fixnum);
  auto table = _table[0];
  
  char backing[table->value_size];
  void * data = backing;
  
  bool vtype(const char * str){
    return strcmp(table->value_type, str) == 0;
  }
  
  if(vtype("vec2")){
    *((vec2 *)data) = cl_object_to_vec2(value);
    
  }else if(vtype("vec3")){
    *((vec3 *)data) = cl_object_to_vec3(value);
  }else if(vtype("vec4")){
    *((vec4 *)data) = cl_object_to_vec4(value);    
  }else if(vtype("f32")){
    *((f32 *) data) = cl_object_to_float(value);
  }else if(vtype("f64")){
    *((f64 *) data) = cl_object_to_double(value);
  }else{
    ASSERT(false);
  }

  auto _key = ecl_to_fixnum(name);
  if(table->key_size == sizeof(u32)){
    u32 key = (u32) _key;
    sorttable_insert(table, &key, data);
  }else if(table->key_size == sizeof(u64)){
    u64 key = (u64)_key;
    sorttable_insert(table, &key, data);
  }else{
    ASSERT(false);
  }
  
  return name;
}

cl_object clgui_get_property(cl_object sortable,cl_object name){
  auto fixnum = ecl_to_fixnum(sortable) + 1;
  sorttable ** _table = index_table_lookup(sortable_table, fixnum);
  auto table = _table[0];
  auto idx = ecl_to_fixnum(name);
  void * data = NULL;
  if(table->key_size == sizeof(u32)){
    u32 index = (u32)idx;
    u64 lutidx = sorttable_find(table, &index);
    data = table->value_area->ptr + lutidx * table->value_size;
  }else if(table->key_size == sizeof(u64)){
    u64 index = (u64)idx;
    u64 lutidx = sorttable_find(table, &index);
    data = table->value_area->ptr + lutidx * table->value_size;
  }else{
    ASSERT(false);
  }

  bool vtype(const char * str){
    return strcmp(table->value_type, str) == 0;
  }
  
  if(vtype("vec2"))
    return vec2_to_cl_object(((vec2 *)data)[0]);
  else if(vtype("vec3"))
    return vec3_to_cl_object(((vec3 *)data)[0]);
  else if(vtype("vec4"))
    return vec4_to_cl_object(((vec4 *)data)[0]); 
  else if(vtype("f32"))
    return float_to_cl_object(((float *)data)[0]);
  else if(vtype("f64"))
    return double_to_cl_object(((double *)data)[0]);
  else{
    logd("Unsupported type: '%s'\n", table->value_type);
    ASSERT(false);
  }
  return name;
  
}
/*
clgui_unset_property(cl_object sortable_ptr, cl_object name){

}*/


cl_object clgui_load_window(cl_object winid){
  u64 id = (u64)cl_object_to_i64(winid);
  make_window(id);
  return ECL_NIL;
}

cl_object clgui_go_draw_windows(){
  //while(true){//!get_should_exit(game_board)){
    u64 window_count = get_count_window_state();
    u64 * windows = get_keys_window_state();
    for(u64 i = 0; i < window_count; i++){
      u64 win_id = windows[i];
      auto method = get_method(win_id, render_control_method);
      u64 ts = timestamp(); // for fps calculation.
      i += 0.03;
      
      //iron_sleep(0.03);
      glfwPollEvents();
      u64 ts2 = timestamp(); // for fps calculation.
      method(win_id);
      {
	u64 tsend = timestamp();
	u64 msdiff = tsend - ts;
	char fpsbuf[100];
	sprintf(fpsbuf, "%3.0f / %3.0f FPS", 1.0 / (1e-6 * (tsend - ts2)), 1.0 / (1e-6 * msdiff));
	//remove_text(fpstextline);
	//set_text(fpstextline, fpsbuf);
      }
    }
    return ECL_NIL;
    //}
}

void gui_demo(){

  
  char * lispChars[] = {(char *)"", (char *)""};
  ecl_set_option(ECL_OPT_TRAP_SIGINT, false);
  ecl_set_option(ECL_OPT_TRAP_SIGFPE, false);
  ecl_set_option(ECL_OPT_TRAP_SIGILL, false);
  ecl_set_option(ECL_OPT_TRAP_SIGSEGV, false);
  ecl_set_option(ECL_OPT_TRAP_INTERRUPT_SIGNAL, false);
  cl_boot(1, lispChars);
  //ecl_enable_interrupts();

  //  auto libname = c_string_to_object("\"test.lisp\"");
  //auto lib = ecl_library_open(libname, true);
  //ecl_library_close(lib);
  cl_eval(c_string_to_object("(load \"clgui.lisp\")"));
  ecl_def_c_function(ecl_make_symbol("STRING-ID", "CL-GUI-BASE"), (cl_objectfn_fixed) clgui_string_id, 1);
  ecl_def_c_function(ecl_make_symbol("GET-TABLE-COUNT", "CL-GUI-BASE"), (cl_objectfn_fixed) clgui_get_sortable_count, 0);
  ecl_def_c_function(ecl_make_symbol("GET-TABLE-DATA", "CL-GUI-BASE"), (cl_objectfn_fixed) clgui_get_sortable_data, 1);
  ecl_def_c_function(ecl_make_symbol("SET-PROPERTY", "CL-GUI-BASE"), (cl_objectfn_fixed) clgui_set_property, 3);
  ecl_def_c_function(ecl_make_symbol("GET-PROPERTY", "CL-GUI-BASE"), (cl_objectfn_fixed) clgui_get_property, 2);
  ecl_def_c_function(ecl_make_symbol("MAKE-WINDOW", "CL-GUI-BASE"), (cl_objectfn_fixed) clgui_load_window, 1);
  ecl_def_c_function(ecl_make_symbol("DRAW-WINDOWS", "CL-GUI-BASE"), (cl_objectfn_fixed) clgui_go_draw_windows, 0);

  //ecl_def_c_function(ecl_make_symbol("STRING-ID", "CL-GUI-BASE"), (cl_objectfn_fixed) clgui_string_id, 1);
  
  

  /*auto symbol = ecl_make_symbol("TEST-CFUN", "TEST");
  
  ecl_def_c_function(symbol, (cl_objectfn_fixed) cfun_test, 1);
  
  cl_eval(c_string_to_object("(print \"hello world\")"));
  cl_eval(c_string_to_object("(print (test:button))"));
  cl_eval(c_string_to_object("(print (test:test))"));
  cl_eval(c_string_to_object("(print (test:test-cfun (test:test)))"));
  cl_eval(c_string_to_object("(print (test:clgui-object \"hello\"))"));
  cl_eval(c_string_to_object("(print (test:clgui-object \"hello2\"))"));
  cl_eval(c_string_to_object("(print (test:clgui-object \"hello\"))"));
  */
  
  init_gui();
  register_sortable(size_initialize());
  //register_sortable(color_initialize());
  sorttable * color_table = color_alpha_initialize();
  color_table->name = "color-alpha";
  
  register_sortable(color_table);
  register_sortable(window_position_initialize());
  cl_eval(c_string_to_object("(load \"prev-test.lisp\")"));
  //cl_eval(c_string_to_object("(handler-bind ((condition (lambda (c) (print \"WTF\")))) (load \"test.lisp\"))"));
  //ecl_enable_interrupts();
  //cl_eval(c_string_to_object("(load \"test.lisp\")"));
  //ecl_disable_interrupts();  
  while(true){//!get_should_exit(game_board)){
    u64 window_count = get_count_window_state();
    u64 * windows = get_keys_window_state();
    for(u64 i = 0; i < window_count; i++){
      u64 win_id = windows[i];
      auto method = get_method(win_id, render_control_method);
      u64 ts = timestamp(); // for fps calculation.
      i += 0.03;
      
      //iron_sleep(0.03);
      glfwPollEvents();
      u64 ts2 = timestamp(); // for fps calculation.
      method(win_id);
      {
	u64 tsend = timestamp();
	u64 msdiff = tsend - ts;
	char fpsbuf[100];
	sprintf(fpsbuf, "%3.0f / %3.0f FPS", 1.0 / (1e-6 * (tsend - ts2)), 1.0 / (1e-6 * msdiff));
	//remove_text(fpstextline);
	//set_text(fpstextline, fpsbuf);
      }
    }	
  }
  //unset_should_exit(game_board);
}
