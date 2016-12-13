#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "sortable.h"
#include "persist_oop.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>

#include "gui.h"
#include "gui_test.h"
#include "index_table.h"
#include "game_board.h"

typedef struct{
  vec3 position;
  u32 model;
}entity_data;

typedef enum{
  KIND_MODEL,
  KIND_POLYGON
}model_kind;

typedef struct{
  model_kind type;
  u32 index;
}model_data;

typedef struct{
  u32 next_index;
  u32 vertex;

}polygon_data;

typedef struct{
  u32 color;
  vec2 position;
}vertex_data;


void print_model_data(u32 index, index_table * models, index_table * polygon, index_table * vertex){
  while(index != 0){
    model_data * m2 = index_table_lookup(models, index);
    index = m2->index;

    switch(m2->type){
    case KIND_MODEL:
      print_model_data(m2->index, models, polygon, vertex);
      break;
    case KIND_POLYGON:
      {
	logd("POLYGON:\n");
	polygon_data * pd = index_table_lookup(polygon, index);
	while(index != 0){
	  if(pd->vertex != 0){
	    vertex_data * vd = index_table_lookup(vertex, pd->vertex);
	    logd("  Vertex: %p ", vd->color);
	    vec2_print(vd->position);
	    logd("\n");
	  }
	  
	  index = pd->next_index;
	  if(index != 0)
	    pd = index_table_lookup(polygon, index);
	}
      }
	
    }
    
  }
}

typedef struct{
  u32 gl_ref;
  u32 count;
}loaded_polygon_data;

/*
typedef index_table polygon_table;
typedef u32 polygon_id;

polygon_id polygon_create(graphics__context * ctx);
u32 polygon_add_vertex2f(graphics__context * ctx, polygon_id polygon, vec3 offset);
void polygon_set_color(graphics__context * ctx, polygon_id polygon, vec4 color);
void polygon_read(graphics__context * ctx, polygon_id polygon, vec3 ** offsets, u64 count);
void polygon_render(graphics__context * ctx, polygon_id polygon, mat3 tform, vec4 color);

auto square = polygon_create(ctx);
polygon_add_vertex(ctx, square, vec2_new(0,0));
polygon_add_vertex(ctx, square, vec2_new(1,0));
polygon_add_vertex(ctx, square, vec2_new(1,1));
polygon_add_vertex(ctx, square, vec2_new(0,1));
polygon_set_color(ctx, square, vec4_new(154.0 / 255.0, 36.0 / 255.0, 31.0 / 255.0, 1));

polygon_render(ctx, square, tform);

*/


CREATE_TABLE_DECL2(active_entities, u32, bool);
CREATE_TABLE_NP(active_entities, u32, bool);
CREATE_TABLE_DECL2(loaded_polygon, u32, loaded_polygon_data);
CREATE_TABLE_NP(loaded_polygon, u32, loaded_polygon_data);
CREATE_TABLE_DECL2(polygon_color, u32, vec4);
CREATE_TABLE2(polygon_color, u32, vec4);

typedef struct{
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
}graphics_context;

typedef u32 polygon_id;
polygon_id polygon_create(graphics_context * ctx);
void polygon_add_vertex2f(graphics_context * ctx, polygon_id polygon, vec2 offset);

void graphics_context_load(graphics_context * ctx){
  memset(ctx, 0, sizeof(*ctx));
  ctx->models = index_table_create(NULL/*"simple/models"*/, sizeof(model_data));
  ctx->polygon = index_table_create(NULL/*"simple/polygon"*/, sizeof(polygon_data));

  ctx->vertex = index_table_create(NULL/*"simple/vertex"*/, sizeof(vertex_data));
  ctx->entities = index_table_create(NULL/*"simple/entities"*/, sizeof(entity_data));
  ctx->polygons_to_delete = index_table_create(NULL, sizeof(u32));
  ctx->gpu_poly = loaded_polygon_table_create(NULL);
  ctx->poly_color = polygon_color_table_create(NULL);
  ctx->active_entities = active_entities_table_create(NULL);
  polygon_id p1 = polygon_create(ctx);
  ctx->pointer_index = p1;
  polygon_add_vertex2f(ctx, p1, vec2_new(0, 0));
}


polygon_id polygon_create(graphics_context * ctx){
  return index_table_alloc(ctx->polygon);
}

void polygon_add_vertex2f(graphics_context * ctx, polygon_id polygon, vec2 offset){

  u32 t1 = index_table_alloc(ctx->vertex);
  vertex_data * v = index_table_lookup(ctx->vertex, t1);
  v->position = offset;
  UNUSED(offset);
  UNUSED(v);
  polygon_data * pd = index_table_lookup(ctx->polygon, polygon);
  if(pd->vertex == 0){
    pd->vertex = t1;
    return;
  }
  while(pd->next_index != 0){
    polygon = pd->next_index;
    pd = index_table_lookup(ctx->polygon, polygon);
  }
  
  polygon_id p1 = polygon_create(ctx);
  pd->next_index = p1;
  pd = index_table_lookup(ctx->polygon, p1);
  pd->vertex = t1;
}


void simple_graphics_load_test(graphics_context * ctx){
  u32 i1 = index_table_alloc(ctx->models);
  
  model_data * m1 = index_table_lookup(ctx->models, i1);
  m1->type = KIND_POLYGON;
  {
    polygon_id p1 = polygon_create(ctx);
    polygon_add_vertex2f(ctx, p1, vec2_new(0,0));
    //polygon_add_vertex2f(ctx, p1, vec2_new(0.8,0.8));
    polygon_color_set(ctx->poly_color, p1, vec4_new(0.5, 1.0, 0.5, 1.0));
    m1->index = p1;
  }
  u32 player = index_table_alloc(ctx->entities);
  entity_data * ed = index_table_lookup(ctx->entities, player);
  ed->position = vec3_new(0,0,0);
  ed->model = i1;

  active_entities_set(ctx->active_entities, player, true);
}


CREATE_TABLE_DECL2(graphics_context, u64, graphics_context);
CREATE_TABLE_NP(graphics_context, u64, graphics_context);
u32 simple_grid_polygon_load(vec2 * vertexes, u32 count){
  u32 vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes[0]) * count, vertexes, GL_STATIC_DRAW);
  logd("Loaded %i bytes to VBO\n", sizeof(vertexes[0]) * count);
  return vbo;
}

typedef struct{
  u32 shader;
  u32 camera_loc;
  u32 color_loc;
  u32 vertex_loc;

}simple_grid_shader;

simple_grid_shader simple_grid_shader_get(){
  static bool initialized = false;
  static u32 shader = 0;
  static u32 camera_loc, color_loc, vertex_loc;
  
  if(!initialized){
    initialized = true;
    char * vs = read_file_to_string("simple_shader.vs");
    char * fs = read_file_to_string("simple_shader.fs");
    shader = load_simple_shader(vs, strlen(vs), fs, strlen(fs));
    dealloc(vs);
    dealloc(fs);
    
    camera_loc = glGetUniformLocation(shader, "camera");
    color_loc = glGetUniformLocation(shader, "color");
    vertex_loc = glGetAttribLocation(shader, "vertex");
    logd("Loading shader..\n");
  }
  simple_grid_shader _shader = {
    .shader = shader,
    .camera_loc = camera_loc,
    .color_loc = color_loc,
    .vertex_loc = vertex_loc
  };
  return _shader;
}

vec2 graphics_context_pixel_to_screen(const graphics_context ctx, vec2 pixel_coords){
  vec2 v = vec2_sub(vec2_scale(vec2_div(pixel_coords, ctx.render_size), 2.0), vec2_one);
  v.y = -v.y;
  return v;
}

void simple_grid_render_gl(const graphics_context ctx, u32 polygon_id, mat4 camera){
  loaded_polygon_data loaded;
  if(false == loaded_polygon_try_get(ctx.gpu_poly, polygon_id, &loaded)) {

    u32 index = polygon_id;
    u32 count = 0;
    for(u32 index = polygon_id; index != 0; index = ((polygon_data *)index_table_lookup(ctx.polygon, index))->next_index)
      count += 1;
    
    
    vec2 positions[count];
      vec2 * p = &positions[0];
      while(index != 0){
	polygon_data * pd = index_table_lookup(ctx.polygon, index);
	index = pd->next_index;
	vertex_data * vd = index_table_lookup(ctx.vertex, pd->vertex);
	*p = vd->position;
	p += 1;
      }
      ASSERT(count > 0);
      loaded.gl_ref = simple_grid_polygon_load(positions, count);
      ASSERT(loaded.gl_ref > 0);
      loaded.count = count;
      loaded_polygon_set(ctx.gpu_poly, polygon_id, loaded);
  }

  glBindBuffer(GL_ARRAY_BUFFER, loaded.gl_ref);  
  simple_grid_shader shader = simple_grid_shader_get();
  
  vec4 color = vec4_zero;
  polygon_color_try_get(ctx.poly_color, polygon_id, &color);
  glUseProgram(shader.shader);
  glUniformMatrix4fv(shader.camera_loc,1,false, &(camera.data[0][0]));
  glUniform4f(shader.color_loc, color.x, color.y, color.z, color.w);
  glEnableVertexAttribArray(shader.vertex_loc);
  glVertexAttribPointer(shader.vertex_loc, 2, GL_FLOAT, false, 0, 0);
  
  glDrawArrays(GL_TRIANGLE_STRIP, 0, loaded.count);
  glPointSize(3);
  glDrawArrays(GL_POINTS, 0, loaded.count);
  glDisableVertexAttribArray(shader.vertex_loc);
}

void simple_grid_render(u64 id){

  graphics_context gd = get_graphics_context(id);
  {
    u64 cnt = 0;
    u32 * items = index_table_all(gd.polygons_to_delete, &cnt);
    if(cnt > 0){
      loaded_polygon_data loaded;
      for(u64 i = 0; i < cnt; i++){
	if(loaded_polygon_try_get(gd.gpu_poly, items[i], &loaded)){
	  u32 ref = loaded.gl_ref;
	  glDeleteBuffers(1, &ref);
	  loaded_polygon_unset(gd.gpu_poly, items[i]);
	}
      }
      index_table_clear(gd.polygons_to_delete);
    }
  }
  
  {
    vec2 offset = graphics_context_pixel_to_screen(gd, gd.pointer_position);
    UNUSED(offset);
    simple_grid_render_gl(gd, gd.pointer_index, mat4_translate(offset.x, offset.y, 0)); 
  }
  
  u32 entities[10];
  bool unused[10];
  u64 idx = 0;
  u64 cnt = 0;
  while(0 != (cnt = active_entities_iter_all(gd.active_entities, entities,unused, array_count(unused), &idx))){
    for(u64 i = 0; i < cnt; i++){
      u32 entity = entities[i];
      entity_data * ed = index_table_lookup(gd.entities, entity);
      if(ed->model != 0){
	model_data * model = index_table_lookup(gd.models, ed->model);
	switch(model->type){
	case KIND_POLYGON:{
	  
	  simple_grid_render_gl(gd, model->index, mat4_identity());
	}
	default:
	  break;
	  
	}

      }
    }
  }

  u32 error = glGetError();
  if(error > 0){
    logd("GL ERROR: %i\n");
  }
  
}

void simple_grid_renderer_create(u64 id){
  graphics_context gd;
  graphics_context_load(&gd);
  set_graphics_context(id, gd);
}

void simple_grid_measure(u64 id, vec2 * size){
  UNUSED(id);
  *size = shared_size;
  graphics_context gd = get_graphics_context(id);
  gd.render_size = *size;
  set_graphics_context(id, gd);
  
}

void simple_grid_mouse_over_func(u64 grid_id, double x, double y, u64 method){
  graphics_context ctx = get_graphics_context(grid_id);
  ctx.pointer_position = vec2_new(x, y);
  set_graphics_context(grid_id, ctx);
  if(method != 0){
    auto on_mouse_over = get_method(grid_id, method);
    on_mouse_over(grid_id, x, y, 0);
    return;
  }
}

void simple_grid_mouse_down_func(u64 grid_id, double x, double y, u64 method){
  UNUSED(method);
  if(mouse_button_action != 1) return;
  graphics_context ctx = get_graphics_context(grid_id);
  vec2 p = graphics_context_pixel_to_screen(ctx, vec2_new(x, y));
  vec2_print(p);logd("\n");
  u32 first_entity = 0;
  bool unused = false;
  u64 idx = 0;
  ASSERT(active_entities_iter_all(ctx.active_entities, &first_entity, &unused, 1, &idx) == 1);
  model_data * m1 = index_table_lookup(ctx.models, first_entity);
  polygon_add_vertex2f(&ctx, m1->index, p);
  loaded_polygon_data loaded;
  if(loaded_polygon_try_get(ctx.gpu_poly, m1->index, &loaded)){
    u32 idx = index_table_alloc(ctx.polygons_to_delete);
    u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
    ptr[0] = m1->index;
  }
}


void simple_grid_initialize(u64 id){
  define_method(id, render_control_method, (method)simple_grid_render);
  define_method(id, measure_control_method, (method)simple_grid_measure);
}

typedef enum{
  SELECTED_ENTITY,
  SELECTED_MODEL,
  SELECTED_POLYGON,
  SELECTED_VERTEX
}selected_kind;

typedef struct{
  selected_kind selection_kind;
  u32 selected_index;
}editor_context;

CREATE_TABLE_DECL2(simple_graphics_editor_context, u64, editor_context);
CREATE_TABLE_NP(simple_graphics_editor_context, u64, editor_context);


void simple_graphics_editor_render(u64 id){

  graphics_context gd = get_graphics_context(id);
  {
    u64 cnt = 0;
    u32 * items = index_table_all(gd.polygons_to_delete, &cnt);
    if(cnt > 0){
      loaded_polygon_data loaded;
      for(u64 i = 0; i < cnt; i++){
	if(loaded_polygon_try_get(gd.gpu_poly, items[i], &loaded)){
	  u32 ref = loaded.gl_ref;
	  glDeleteBuffers(1, &ref);
	  loaded_polygon_unset(gd.gpu_poly, items[i]);
	}
      }
      index_table_clear(gd.polygons_to_delete);
    }
  }
  
  {
    vec2 offset = graphics_context_pixel_to_screen(gd, gd.pointer_position);
    UNUSED(offset);
    simple_grid_render_gl(gd, gd.pointer_index, mat4_translate(offset.x, offset.y, 0)); 
  }
  
  u32 entities[10];
  bool unused[10];
  u64 idx = 0;
  u64 cnt = 0;
  while(0 != (cnt = active_entities_iter_all(gd.active_entities, entities,unused, array_count(unused), &idx))){
    for(u64 i = 0; i < cnt; i++){
      u32 entity = entities[i];
      entity_data * ed = index_table_lookup(gd.entities, entity);
      if(ed->model != 0){
	model_data * model = index_table_lookup(gd.models, ed->model);
	switch(model->type){
	case KIND_POLYGON:{
	  
	  simple_grid_render_gl(gd, model->index, mat4_identity());
	}
	default:
	  break;
	  
	}

      }
    }
  }

  u32 error = glGetError();
  if(error > 0){
    logd("GL ERROR: %i\n");
  }
}

bool is_whitespace(char c){
  return c == ' ' || c == '\t';
  
}

bool copy_nth(const char * _str, u32 index, char * buffer, u32 buffer_size){
  char * str = (char *) _str;
  while(*str != 0){
    while(is_whitespace(*str))
      str++;
    
    while(*str != 0 && is_whitespace(*str) == false){
      if(index == 0){
	*buffer = *str;
	buffer_size -= 1;
	buffer++;
      }
      str++;
    }
    if(index == 0){
      *buffer = 0;
      return true;
    }
    index--;
  }
  return false;

}

CREATE_TABLE_DECL2(simple_graphics_control, u64, u64);
CREATE_TABLE_NP(simple_graphics_control, u64, u64);

static void command_entered(u64 id, char * command){
  u64 control = get_simple_graphics_control(id);
  editor_context editor = get_simple_graphics_editor_context(control);
  graphics_context ctx = get_graphics_context(control);
  char first_part[100];
  bool first(const char * check){
    return strcmp(first_part, check) == 0;
  }

  char snd_part[100];
  bool snd(const char * check){
    return strcmp(snd_part, check) == 0;
  }
  
  if(copy_nth(command, 0, first_part, array_count(first_part))){
    logd("At least one arg: %s\n", first_part);
    if(first("exit")){
	logd("EXIT!\n");
	set_should_exit(control, true);
    }else if(first("list")){
      logd("Listing.. \n");
      u64 cnt = 0;
      entity_data * entities = index_table_all(ctx.entities, &cnt);
      logd("Entity CNT:%i\n", cnt);
      for(u64 i = 0; i < cnt; i++){
	u32 entity_id = i + 1;

	entity_data * entity = entities + i;
	logd("Entity: %i\n", entity_id);
	u32 model_id = entity->model;
	if(model_id != 0){
	  logd("Model: %i\n", model_id);
	  model_data * md = index_table_lookup(ctx.models, model_id);
	  u32 polygon_id = md->index;
	  if(md->type == KIND_POLYGON && polygon_id != 0){
	    logd("Polygon: %i\n", polygon_id);
	    polygon_data * pd = index_table_lookup(ctx.polygon, polygon_id);
	    logd("P: %p\n", pd);
	    // fix data structure
	    
	  }
	}
      }
    }else if(first("create")){
      copy_nth(command, 1, snd_part, array_count(snd_part));
      logd("SND: %s\n", snd_part);
      if(snd("entity")){
	u32 i1 = index_table_alloc(ctx.entities);
	editor.selection_kind = SELECTED_ENTITY;
	editor.selected_index = i1;
	logd("Created entity: %i\n", i1);
      }else if(snd("model") && editor.selection_kind == SELECTED_ENTITY){
	u32 i1 = index_table_alloc(ctx.models);
	entity_data * entity = index_table_lookup(ctx.entities, editor.selected_index);
	editor.selection_kind = SELECTED_MODEL;
	editor.selected_index = i1;
	entity->model = i1;
      }else if(snd("polygon") && editor.selection_kind == SELECTED_MODEL){
	logd("creating polygon..\n");
	polygon_id p1 = polygon_create(&ctx);
	editor.selection_kind = SELECTED_POLYGON;
	editor.selected_index = p1;
	model_data * entity = index_table_lookup(ctx.models, editor.selected_index);
	entity->type = KIND_POLYGON;
	entity->index = p1;
      }
      set_simple_graphics_editor_context(control, editor);
      logd("%i %i\n", editor.selection_kind, editor.selected_index);
    }else{
      logd("Unkown command!\n");
    }
  }
  /*u64 editor = get_parent(id);
  if(starts_with(command, "create ")){
    char buffer[100];
    if(copy_nth(command, 1, buffer, array_count(buffer))){


    }
    }*/
  logd("COMMAND ENTERED %i %s\n", id, command);

}

#include "console.h"
void simple_graphics_editor_load(u64 id, u64 win_id){
  define_method(id, render_control_method, (method)simple_graphics_editor_render);
  define_method(id, measure_control_method, (method)simple_grid_measure);
  define_method(id, mouse_over_method, (method)simple_grid_mouse_over_func);
  define_method(id, mouse_down_method, (method) simple_grid_mouse_down_func);
  
  u64 console = intern_string("console");
  set_simple_graphics_control(console, id);
  set_focused_element(win_id, console);
  set_console_height(console, 300);
  create_console(console);
  add_control(win_id, console);
  set_margin(console, (thickness){5,5,5,5});
  set_vertical_alignment(console, VALIGN_BOTTOM);
  set_console_history_cnt(console, 100);
  define_method(console, console_command_entered_method, (method)command_entered);
  
}
