#include <GLFW/glfw3.h>
#include <iron/full.h>
#include "persist.h"
#include "persist_oop.h"
#include "sortable.h"

CREATE_MULTI_TABLE2(base_class, u64, u64);

void define_subclass(u64 class, u64 base_class){
  set_base_class(class, base_class);
}

u64 get_baseclass(u64 class, u64 * index){
  u64 r = 0;
  iter_base_class(&class, 1, &class, &r, 1, index);
  if(r != 0)
    return *ref_at_base_class(r);
  return 0;
}

void define_method(u64 class_id, u64 method_id, method handler){
  attach_handler(class_id, method_id, handler);
}

method get_method(u64 class_id, u64 method_id){
  if(class_id == 0)
    return NULL;
  //ASSERT(class_id != 0);
  method cmd = get_command_handler(class_id, method_id);
  if(cmd == NULL){
    u64 idx = 0;
  next_cls:;
    u64 baseclass = get_baseclass(class_id, &idx);
    
    if(baseclass != 0){
      method handler = get_method(baseclass, method_id);
      if(handler == NULL)
	goto next_cls;
      return handler;
    }
  }
  return cmd;
}

class * new_class(u64 id){
  return find_item("classes", id, sizeof(class), true);
}

method get_command_handler(u64 control_id, u64 command_id){
  method * item = get_command_handler2(control_id, command_id, false);
  return item == NULL ? NULL : *item;
}

method * get_command_handler2(u64 control_id, u64 command_id, bool create){
  static method handlers[100] = {};
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

void * find_item(const char * table, u64 itemid, u64 size, bool create){
  u64 total_size = 0;
  void * w = persist_alloc2(table, size, &total_size);
  u32 cnt = total_size / size;
  void * free = NULL;
  {
    void * ptr = w;
    for(u32 i = 0; i < cnt; i++){
      u64 * id = ptr;
      ASSERT(*id < 5000);
      if(*id == itemid){
	return ptr;
      }else if(*id == 0 && free == NULL){
	free = ptr;
	break;
      }
      ptr += size;	
     
    }
  }
  if(!create) return NULL;
  if(free == NULL){

    w = persist_realloc(w, size * (cnt + 1));
    free = w + cnt * size;
  }
  
  u64 * id = free;
  *id = itemid;
  return free;
}
