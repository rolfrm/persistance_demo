void simple_graphics_editor_load(u64 id, u64 win_id);
bool simple_graphics_editor_test();

typedef struct{
  vec3 position;
  u32 model;
}entity_data;

typedef enum{
  KIND_MODEL,
  KIND_POLYGON
}model_kind;

typedef struct{
  index_table_sequence polygons;
}model_data;

typedef struct{
  index_table_sequence vertexes;
  u32 material;
  f32 physical_height;
}polygon_data;

typedef struct{
  u32 color;
  vec2 position;
}vertex_data;

typedef struct{
  u32 entity;
  u32 model;
  u32 polygon_offset;
  u32 vertex_offset;
  u32 material;
}entity_local_data;

typedef struct{
  entity_local_data entity1;
  entity_local_data entity2;
}collision_data;

typedef struct{
  char data[100];
}name_data;


typedef enum{
  ENTITY_TYPE_PLAYER = 1,
  ENTITY_TYPE_ENEMY = 2,
  ENTITY_TYPE_ITEM = 3,
  ENTITY_TYPE_INTERACTABLE = 4
}ENTITY_TYPE;

#define GROUP_INTERACTION 10

CREATE_TABLE_DECL2(entity_type, u64, ENTITY_TYPE);
CREATE_TABLE_DECL2(desc_text, u64, name_data);
CREATE_TABLE_DECL2(hit_queue, u32, u32);

struct _graphics_context;
typedef struct _graphics_context graphics_context;

typedef enum{
  INTERACTION_NONE,
  INTERACTION_DONE,
  INTERACTION_WAIT,
  INTERACTION_ERROR,
}interaction_status;

typedef interaction_status (* interact_fcn)(graphics_context *ctx, u32 interactor, u32 interactee, bool checkavail);


typedef enum{
  SELECTED_ENTITY,
  SELECTED_MODEL,
  SELECTED_POLYGON,
  SELECTED_VERTEX,
  SELECTED_NONE
}selected_kind;

typedef struct{
  selected_kind selection_kind;
  u32 selected_index;
  float zoom;
  u32 focused_item;
  selected_kind focused_item_kind;
  vec2 offset;
  bool is_grabbed;
  union {
    struct {
      bool snap_to_grid;
    };
    
    u32 reserved[16];
  };
}editor_context;

typedef bool (* simple_graphics_editor_fcn)(graphics_context * ctx, editor_context * editor, char * command);
typedef void (* simple_game_update_fcn)(graphics_context * ctx);
void set_desc_text2(u32 idx, u32 group, const char * str);
bool get_desc_text2(u32 idx, u32 group, char * buf, u32 bufsize);
CREATE_TABLE_DECL2(active_entities, u32, bool);
CREATE_TABLE_DECL2(polygon_color, u32, vec4);
CREATE_TABLE_DECL2(current_impulse, u64, vec3);
CREATE_TABLE_DECL2(entity_2_collisions, u32, u32);

typedef struct{
  u32 gl_ref;
  u32 count;
  float depth; // for flat polygons only.
}loaded_polygon_data;

CREATE_TABLE_DECL2(loaded_polygon, u32, loaded_polygon_data);
CREATE_TABLE_DECL2(simple_game_interactions, u32, interact_fcn);
CREATE_TABLE_DECL2(simple_game_editor_fcn, u32, simple_graphics_editor_fcn);
CREATE_TABLE_DECL2(simple_game_update, u32, simple_game_update_fcn);
CREATE_TABLE_DECL2(ghost_material, u32, bool);

typedef enum{
  GAME_EVENT_MOUSE_BUTTON
}game_event_kind;

typedef struct{
  game_event_kind kind;
  union{
    struct{
      int button;
      vec2 game_position;
      bool pressed;
    }mouse_button;
    char reserved[28];
  };
}game_event;

u32 game_event_index_new();
CREATE_TABLE_DECL2(game_event, u32, game_event);

struct _graphics_context{
  index_table * entities;
  index_table * models;
  index_table * polygon;
  index_table * vertex;
  loaded_polygon_table * gpu_poly;
  index_table * polygons_to_delete;
  polygon_color_table * poly_color;
  active_entities_table * active_entities;
  vec2 render_size;
  vec2 pointer_position;
  u32 pointer_index;
  index_table * collision_table;
  entity_2_collisions_table * collisions_2_table;
  index_table * loaded_modules;
  simple_game_interactions_table * interactions;
  simple_game_editor_fcn_table * editor_functions;
  simple_game_update_table * game_update_functions;
  ghost_material_table * ghost_table;
};


void graphics_context_load_interaction(graphics_context * ctx, interact_fcn f, u32 id);
void graphics_context_load_update(graphics_context * ctx, simple_game_update_fcn f, u32 id);
void simple_game_editor_load_func(graphics_context * ctx, simple_graphics_editor_fcn f, u32 id);
void simple_game_point_collision(graphics_context ctx, u32 * entities, u32 entity_count, vec2 loc, index_table * collisiontable);
