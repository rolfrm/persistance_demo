#include <GLFW/glfw3.h>
#include <iron/full.h>
#include "persist.h"
#include "sortable.h"
#include "persist_oop.h"

CREATE_MULTI_TABLE2(base_class, u64, u64);

bool is_subclass(u64 class, u64 base_class){
  u64 r[10];
  u64 index = 0;
  u64 cnt;
  while(0 < (cnt = iter2_base_class(class, r, array_count(r), &index))){
    if(cnt == 0)
      return false;
    for(u64 i = 0; i < cnt; i++){
      if(r[i] == base_class)
	return true;
      
    }
    for(u64 i = 0; i < cnt; i++){
      if(is_subclass(r[i], base_class))
	return true;
    }
  }
  return false;
}

void define_subclass(u64 class, u64 base_class){
  if(!is_subclass(class, base_class))
    set_base_class(class, base_class);
}

u64 get_baseclass(u64 class, u64 * index){
  u64 r = 0;
  iter_base_class(&class, 1, NULL, &r, 1, index);

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
  UNUSED(create);
  static sorttable * tab = NULL;
  if(tab == NULL)
    tab = IRON_CLONE(create_sorttable(sizeof(u128), sizeof(method), NULL));
  union{
    struct {
      u64 _id, _command_id;
    };
    u128 index;
  }key;
  key._id = control_id;
  key._command_id = command_id;
  u64 idx = sorttable_find(tab, &key.index);
  if(idx == 0)
    idx = sorttable_insert_key(tab, &key.index);
  method * mtab = tab->value_area->ptr;
  return mtab + idx;
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

u64 intern_string(const char *);

void test_persist_oop(){
  u64 scls = intern_string("cls2");
  u64 cls = intern_string("test cls");
  u64 obj1 = intern_string("test obj");
  define_subclass(cls, scls);
  define_subclass(obj1, cls);
  u64 i = 0;
  ASSERT(get_baseclass(cls, &i) == scls);
  u64 mth1 = intern_string("test method");
  u64 mth2 = intern_string("test method2");
  
  int called = 0;
  void was_called(){
    called += 1;
  }
  bool called2 = 0;
  void was_called2(){
    called2 += 1;
  }
  
  define_method(cls, mth1, (method) was_called);
  define_method(scls, mth2, (method)was_called2);


  
  CALL_BASE_METHOD(obj1, mth1, obj1);
  CALL_BASE_METHOD(obj1, mth2, obj1);
  //get_method(cls, mth1)(cls);
  //get_method(cls, mth2)(cls);
  logd("CALLED: %i\n", called);
  ASSERT(called == 1);
  ASSERT(called2 == 1);
} 
