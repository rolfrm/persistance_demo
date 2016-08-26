#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
#include "sortable.h"
#include "persist_oop.h"
#include "gui.h"
#include "console.h"

CREATE_TABLE(console_history_cnt, u64, u64);

size_t codepoint_to_utf8(u32 codepoint, char * out, size_t maxlen){
  char data[4];
  u32 c = codepoint;
  char * b = data;
  if (c<0x80) *b++=c;
  else if (c<0x800) *b++=192+c/64, *b++=128+c%64;  
  else if (c-0xd800u <0x800) return 0;
  else if (c<0x10000) *b++=224+c/4096, *b++=128+c/64%64, *b++=128+c%64;
  else if (c<0x110000) *b++=240+c/262144, *b++=128+c/4096%64, *b++=128+c/64%64, *b++=128+c%64;
  else return 0;
  
  size_t l = b - data;
  if(maxlen < l ) return 0;
  if(out != NULL)
    memcpy(out, data, l);
  return l;		    
}

u32 utf8_to_codepoint(const char * _txt, size_t * out_len){
  u8 * txt = (u8 *) _txt;
  *out_len = 0;
  if(*txt < 0x80){
    *out_len = 1;
    return *txt;
  }
  int len = 0;
  u32 codept = 0;
  u32 offset = 0;
  if(*txt > 0b11110000){
    len = 4;
    codept = *txt & 0b00000111;
    offset = 0x10000;
  }else if(*txt > 0b11100000){
    len = 3;
    codept = *txt & 0b00001111;
    offset = 0xFFFF;
  }else if(*txt > 0b11000000){
    len = 2;
    offset = 0x0080;
    codept = (*txt & 0b00011111);
  }
  *out_len = (size_t)len;
  for(int i = 1; i < len; i++)
    codept = (codept << 6) | (txt[i] & 0b00111111);
  
  UNUSED(offset);
  return codept;
}

u64 console_class = 0;
u64 console_command_entered_method = 0;
u64 console_add_history_method = 0;

CREATE_TABLE(console_height, u64, float);
CREATE_MULTI_TABLE(console_history, u64, u64);
CREATE_STRING_TABLE(strings, u64);

void render_console(u64 id){

  u64 histcnt = get_console_history_cnt(id);
  float h = get_console_height(id);
  vec2 rect_offset = shared_offset;
  vec2 rect_size = shared_size;
  rect_size.y = MIN(rect_size.y, h);
  rect_render(vec3_new(1,1,1), rect_offset, rect_size);
  
  u64 history[histcnt];
  u64 history_cnt = get_console_history(id, history, array_count(history));
  float voffset = 2;
  char strbuf[100];

  for(i64 i = -1; i < (i64) history_cnt && voffset < h ;i++){
    strbuf[0] = 0;
    
    if(i == -1){
      
      int offset = 0;
      offset = sprintf(strbuf, ">>> ");
      u64 ts = (timestamp() / 500000) % 2;
      
      get_text(id, strbuf + offset, sizeof(strbuf) - offset);
      int l = strlen(strbuf);
      if(ts == 0){
	strbuf[l] = '|';
	strbuf[l+1] = 0;
      }
      
    }else{
      get_strings(history[history_cnt - i - 1], strbuf, sizeof(strbuf));
    }
    vec2 s = measure_text(strbuf, sizeof(strbuf));
    vec2 offset = shared_offset;
    shared_offset = vec2_new(offset.x, shared_offset.y + h - s.y - voffset);
    render_text(strbuf, sizeof(strbuf));
    shared_offset = offset;
    voffset += s.y + 1;
  }
}

void measure_console(u64 id, vec2 * size){
  *size = vec2_new(100, get_console_height(id));
}

void char_handler_console(u64 id, int codepoint, int mods){
  UNUSED(mods);
  int utf8len = codepoint_to_utf8(codepoint, NULL, 100);
  if(!utf8len){
    logd("Error parsing codepoint: %i \n", codepoint);
  }
  ASSERT(utf8len);
  int len = get_text(id, NULL, 1000);
  char buffer[len + utf8len];
  buffer[0] = 0;
  get_text(id, buffer, len);
  int slen = strlen(buffer);
  ASSERT(codepoint_to_utf8(codepoint, buffer + slen, utf8len));
  buffer[slen + utf8len] = 0;
  remove_text(id);
  set_text(id, buffer);
}

void push_console_history(u64 id, const char * buffer){
  u64 str1 = get_unique_number();
  set_strings(str1, buffer);
  add_console_history(id, str1);
  u64 histcnt = get_console_history_cnt(id);
  u64 history[histcnt + 1];
  u64 history_cnt = get_console_history(id, history, array_count(history));
  if(history_cnt > histcnt){
    clear_console_history(id);
    remove_strings(history[0]);
    for(u64 i = 1; i < histcnt + 1; i++){
      add_console_history(id, history[i]);

    }
  }
}

void key_handler_console(u64 id, int key, int mods, int action){
  UNUSED(mods);
  if(key == key_enter && action == key_press){
    int len = get_text(id, NULL, 1000);
    
    char buffer[len];
    get_text(id, buffer, len);
    for(int i = 0; i < len; i++)
      if(buffer[i] == '\n'){
	buffer[i] = 0;
	break;
      }
    logd("Exec buffer: '%s'\n", buffer);
    if(buffer[0] == 0){
      set_text(id, "");
      return;
    }
      
    push_console_history(id, buffer);
    
    remove_text(id);
    set_text(id, "");
    method cmd_entered = get_method(id, console_command_entered_method);
    if(cmd_entered != NULL){
      cmd_entered(id, buffer);
    }
  }else if (key == key_backspace && action != key_release){
    
    int len = get_text(id, NULL, 1000);
    char buffer[len];
    buffer[0] = 0;
    get_text(id, buffer, len);

    char * lastchar = buffer;
    size_t lastlen = 0;
    
    for(int i = 0 ; i < len;){
      if(buffer[i] == 0) break;
      size_t l = 0;
      utf8_to_codepoint(lastchar, &l);
      lastchar = buffer + i;
      lastlen = l;
      i += l;
    }
    memset(lastchar, 0, lastlen);
    
    remove_text(id);
    set_text(id, buffer);
  }
}

static void init_console(){
  if(console_class != 0) return;
  console_class = intern_string("console_class");
  console_command_entered_method = intern_string("command entered");
  define_method(console_class, render_control_method, (method) render_console);
  define_method(console_class, measure_control_method, (method) measure_console);
  define_method(console_class, key_handler_method, (method) key_handler_console);
  define_method(console_class, char_handler_method, (method) char_handler_console);
}

void create_console(u64 id){
  init_console();
  define_subclass(id, console_class);
  set_console_history_cnt(id, 50);
}
