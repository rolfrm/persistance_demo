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
window * find_window(u64 id);
window * make_window(u64 id);

CREATE_TABLE_DECL(margin, u64, thickness);
CREATE_TABLE_DECL(corner_roundness, u64, thickness);
CREATE_TABLE_DECL(color, u64, vec3);
CREATE_STRING_TABLE_DECL(text, u64);
CREATE_STRING_TABLE_DECL(name, u64);
CREATE_MULTI_TABLE_DECL(inventory, u64, u64);
u64 get_unique_number();
u64 intern_string(const char * name);
control_pair * add_control(u64 id, u64 other_id);
control_pair * get_control_pair_parent(u64 parent_id, u64 * index);
rectangle * get_rectangle(u64 id);
rectangle * find_rectangle(u64 id, bool create);
void rect_render(vec3 color, vec2 offset, vec2 size);
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

extern int key_backspace;
extern int key_enter;
extern int key_space;
extern int key_release;
extern int key_press;
extern int key_repeat;
extern int mod_ctrl;

