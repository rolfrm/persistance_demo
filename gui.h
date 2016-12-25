typedef struct{
  int width, height;
  int x, y;
  bool initialized;
  char title[64];
}window;

typedef struct{
  u64 id;
  char name[64 - sizeof(u64)];
}named_item;

typedef enum{
  ORIENTATION_HORIZONTAL,
  ORIENTATION_VERTICAL
}stackpanel_orientation;

typedef struct{
  u64 id;
  stackpanel_orientation orientation;
}stackpanel;

typedef struct{
  float left, up, right, down;
}thickness;

typedef struct{
  u64 id;
  vec2 size;
  vec3 color;
}rectangle;

typedef struct{
  u64 parent_id;
  u64 child_id;
}control_pair;

typedef void (* render_prototype)(u64 id);
typedef void (* measure_prototype)(u64 id, vec2 * size);
typedef void (* char_handler_prototype)(u64 id, int codepoint, int mods);
typedef void (* key_handler_prototype)(u64 id, int key, int mods, int action);

bool once(u64 itemid);

extern u64 window_close_method;
extern u64 mouse_over_method;
extern u64 render_control_method;
extern u64 measure_control_method;
extern u64 mouse_down_method;
extern u64 char_handler_method;
extern u64 key_handler_method;


extern u64 ui_element_class;
extern u64 button_class;
extern u64 window_class;
extern u64 stack_panel_class;
extern u64 rectangle_class;
extern u64 textline_class;
extern __thread vec2 shared_offset, shared_size, window_size;
extern __thread bool mouse_button_action;
extern __thread u32 mouse_button_button;
extern const u32 mouse_button_left;
extern const u32 mouse_button_right;
extern const u32 mouse_button_middle;
extern const u32 mouse_button_press;
extern const u32 mouse_button_release;
extern const u32 mouse_button_repeat;

void make_window(u64 id);

CREATE_TABLE_DECL(margin, u64, thickness);
CREATE_TABLE_DECL(corner_roundness, u64, thickness);
CREATE_TABLE_DECL(color, u64, vec3);
CREATE_STRING_TABLE_DECL(text, u64);
CREATE_STRING_TABLE_DECL(name, u64);
CREATE_MULTI_TABLE_DECL(inventory, u64, u64);
CREATE_TABLE_DECL2(window_state, u64, window);
CREATE_TABLE_DECL2(mouse_hidden, u64, bool);
u64 get_unique_number();
u64 intern_string(const char * name);
control_pair * add_control(u64 id, u64 other_id);
control_pair * get_control_pair_parent(u64 parent_id, u64 * index);
u64 control_pair_get_parent(u64 child_id);
control_pair * gui_get_control(u64 id, u64 other_id);
rectangle * get_rectangle(u64 id);
rectangle * find_rectangle(u64 id, bool create);
void rect_render(vec3 color, vec2 offset, vec2 size);
void rect_render2(vec3 color, vec2 offset, vec2 size, i32 tex, vec2 uv_offset, vec2 uv_scale);
u64 get_textline(u64 id);
stackpanel * get_stackpanel(u64 id);
void init_gui();

typedef enum _horizontal_alignment{
  HALIGN_LEFT = 0,
  HALIGN_CENTER,
  HALIGN_STRETCH,
  HALIGN_RIGHT
  
} horizontal_alignment;

typedef enum _vertical_alignment{
  VALIGN_TOP = 0,  
  VALIGN_BOTTOM,
  VALIGN_STRETCH,
  VALIGN_CENTER
} vertical_alignment;
  
CREATE_TABLE_DECL(vertical_alignment, u64, vertical_alignment);
CREATE_TABLE_DECL(horizontal_alignment, u64, horizontal_alignment);
CREATE_TABLE_DECL(focused_element, u64, u64);
vec2 measure_sub(u64 item);
void render_sub(u64 id);
named_item unintern_string(u64 id);

void render_text(const char * text, size_t len);
vec2 measure_text(const char * text, size_t len);

u32 utf8_to_codepoint(const char * _txt, size_t * out_len);
size_t codepoint_to_utf8(u32 codepoint, char * out, size_t maxlen);

extern const int key_backspace;
extern const int key_enter;
extern const int key_space;
extern const int key_release;
extern const int key_press;
extern const int key_repeat;
extern const int mod_ctrl;
extern const int key_up;
extern const int key_down;
extern const int key_right;
extern const int key_left;
extern const int key_tab;
named_item * get_named_item(const char * table, const char * name, bool create);
u64 get_unique_number();

void set_margin(u64 itemid, thickness t);
thickness get_margin(u64 itemid);

control_pair * add_control(u64 itemid, u64 subitem);

void measure_child_controls(u64 control, vec2 * size);

void handle_mouse_over(u64 control, double x, double y, u64 method);

u64 intern_string(const char * name);
void init_gui();

stackpanel * get_stackpanel(u64 id);
stackpanel * find_stackpanel(u64 id);

void gui_set_mouse_position(u64 window, double x, double y);
extern const u32 gui_window_cursor_hidden;
extern const u32 gui_window_cursor_normal;
extern const u32 gui_window_cursor_disabled;
void gui_set_cursor_mode(u64 window, u32 mode);
void gui_acquire_mouse_capture(u64 window, u64 control);
void gui_release_mouse_capture(u64 window, u64 control);

