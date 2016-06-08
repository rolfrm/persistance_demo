typedef struct{
  bool active;
  u64 base_class;
  u64 class;
}subclass;

typedef struct{
  u64 id;
  bool active;
}class;

typedef void (* method)(u64 control, ...);

subclass * define_subclass(u64 class, u64 base_class);
u64 get_baseclass(u64 class, u64 * index);
void define_method(u64 class_id, u64 method_id, method handler);
method get_method(u64 class_id, u64 method_id);

class * new_class(u64 id);
method get_command_handler(u64 control_id, u64 command_id);

method * get_command_handler2(u64 control_id, u64 command_id, bool create);
void attach_handler(u64 control_id, u64 command_id, void * handler);
