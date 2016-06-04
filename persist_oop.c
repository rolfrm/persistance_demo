#include <GLFW/glfw3.h>
#include <iron/full.h>
#include "persist.h"
#include "persist_oop.h"

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
      command_handler handler = get_method(baseclass, method_id);
      if(handler == NULL)
	goto next_cls;
      return handler;
    }
  }
  return cmd;
}

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


command_handler get_command_handler(u64 control_id, u64 command_id){
  command_handler * item = get_command_handler2(control_id, command_id, false);
  return item == NULL ? NULL : *item;
}

typedef void (* command_handler)(u64 control, ...);
command_handler * get_command_handler2(u64 control_id, u64 command_id, bool create){
  static command_handler handlers[100] = {};
  static bool inited[100] ={};
  static u64 control_ids[100] = {};
  static u64 command_ids[1000] = {};
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
