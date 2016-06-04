typedef struct{
  bool active;
  u64 id;
  char name[32 - sizeof(u64) - sizeof(bool)];
}named_item;

named_item * get_named_item(const char * table, const char * name, bool create);
u64 get_unique_number();

typedef struct{
  bool active;
  u64 id;
}control;

typedef struct{
  bool active;
  u64 id;
  char text[64];
}textline;

typedef enum{
  ORIENTATION_HORIZONTAL,
  ORIENTATION_VERTICAL
}stackpanel_orientation;

typedef struct{
  bool active;
  u64 id;
  stackpanel_orientation orientation;
}stackpanel;

typedef struct{
  float left, up, right, down;
}thickness;

typedef struct{
  bool active;
  u64 id;
  vec2 size;
  vec3 color;
}rectangle;

typedef struct{
  bool active;
  u64 parent_id;
  u64 child_id;
}control_pair;

u64 internalize(const char * name);
u64 internalize_add(u64 intern, u64 addition);

control * get_control(const char * name);
control * find_control(u64 id, bool create);

void set_margin(u64 itemid, thickness t);
thickness get_margin(u64 itemid);

control_pair * add_control(u64 itemid, u64 subitem);

void render_gui_window(u64 windowid);
void load_window(u64 id);
GLFWwindow * find_glfw_window(u64 id);
window * get_window_glfw(GLFWwindow * window);

stackpanel * get_stackpanel(const char * name);
stackpanel * find_stackpanel(u64 id);
void render_stackpanel(stackpanel * stk);

void test_gui();
