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
  index_table_sequence polygons;
}model_data;

typedef struct{
  index_table_sequence vertexes;
}polygon_data;

typedef struct{
  u32 color;
  vec2 position;
}vertex_data;


void print_model_data(u32 index, index_table * models, index_table * polygon, index_table * vertex){
  model_data * m2 = index_table_lookup(models, index);
  for(u32 i = 0; i < m2->polygons.count; i++){
    u32 index = i + m2->polygons.index;
    logd("POLYGON:\n");
    polygon_data * pd = index_table_lookup(polygon, index);
    vertex_data * vd = index_table_lookup_sequence(vertex, pd->vertexes);
    logd("  Vertex: %p ", vd->color);
    vec2_print(vd->position);
    logd("\n"); 
  }
}

typedef struct{
  u32 gl_ref;
  u32 count;
}loaded_polygon_data;

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

  polygon_data * pd = index_table_lookup(ctx->polygon, polygon);
  u32 pcnt = pd->vertexes.count;
  index_table_resize_sequence(ctx->vertex, &(pd->vertexes), pcnt + 1);
  
  vertex_data * v = index_table_lookup_sequence(ctx->vertex, pd->vertexes);
  v[pcnt].position = offset;
}

u32 vertex_get_polygon(graphics_context ctx, u32 vertex){
  u64 polycnt = 0;
  polygon_data * pd = index_table_all(ctx.polygon, &polycnt);
  for(u64 i = 0; i < polycnt; i++){
    u32 offset = pd[i].vertexes.index;
    u32 count = pd[i].vertexes.count;
    logd("cmp: %i %i   %i\n", offset, count, vertex);
    if(offset <= vertex && vertex < offset + count){
      return i + 1;
    }
  }
  return 0;
}


void simple_graphics_load_test(graphics_context * ctx){
  u32 i1 = index_table_alloc(ctx->models);
  
  model_data * m1 = index_table_lookup(ctx->models, i1);
  {
    polygon_id p1 = polygon_create(ctx);
    polygon_add_vertex2f(ctx, p1, vec2_new(0,0));
    //polygon_add_vertex2f(ctx, p1, vec2_new(0.8,0.8));
    polygon_color_set(ctx->poly_color, p1, vec4_new(0.5, 1.0, 0.5, 1.0));
    m1->polygons = (index_table_sequence){.index = p1, .count = 1};
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
  ASSERT(polygon_id != 0);
  if(false == loaded_polygon_try_get(ctx.gpu_poly, polygon_id, &loaded)) {
    polygon_data * pd = index_table_lookup(ctx.polygon, polygon_id);
    u32 count = pd->vertexes.count;
    if(count == 0)
      return;
    vec2 positions[count];
    

    vertex_data * vd = index_table_lookup_sequence(ctx.vertex, pd->vertexes);
    for(u32 i = 0; i < count; i++)
      positions[i] = vd[i].position;
    
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
	for(u32 j = 0; j < model->polygons.count; j++){
	  u32 index = j + model->polygons.index;	
	  simple_grid_render_gl(gd, index, mat4_identity());
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

void simple_grid_mouse_down_func(u64 grid_id, double x, double y, u64 method){
  UNUSED(method);
  if(mouse_button_action != 1) return;
  graphics_context ctx = get_graphics_context(grid_id);
  editor_context editor = get_simple_graphics_editor_context(grid_id);
  vec2 p = graphics_context_pixel_to_screen(ctx, vec2_new(x, y));
  if(editor.selection_kind == SELECTED_VERTEX){
    vertex_data * vd = index_table_lookup(ctx.vertex, editor.selected_index);
    vd->position = p;
    u32 polygon = vertex_get_polygon(ctx, editor.selected_index);
    
    logd(" FOUND POLYGON %i\n", polygon);
    
    loaded_polygon_data loaded;
    if(loaded_polygon_try_get(ctx.gpu_poly, polygon, &loaded)){
      u32 idx = index_table_alloc(ctx.polygons_to_delete);
      u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
      ptr[0] = polygon;
    }
    
  }else if(editor.selection_kind == SELECTED_POLYGON){
    polygon_add_vertex2f(&ctx, editor.selected_index, p);
    loaded_polygon_data loaded;
    if(loaded_polygon_try_get(ctx.gpu_poly, editor.selected_index, &loaded)){
      u32 idx = index_table_alloc(ctx.polygons_to_delete);
      u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
      ptr[0] = editor.selected_index;
    }
  }
}


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
	for(u32 j = 0; j < model->polygons.count; j++){
	  u32 index = j + model->polygons.index;
	  simple_grid_render_gl(gd, index, mat4_identity());
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
	  for(u32 j = 0; j < md->polygons.count; j++){
	    u32 polygon_id = j + md->polygons.index;
	    logd("Polygon: %i\n", polygon_id);
	    polygon_data * pd = index_table_lookup(ctx.polygon, polygon_id);
	    logd("P: %p\n", pd);
	    for(u32 i = 0; i < pd->vertexes.count; i++){
	      vertex_data * vd = index_table_lookup(ctx.vertex, pd->vertexes.index + i);
	      logd("Vertex %i: ", pd->vertexes.index + i);vec2_print(vd->position);logd("\n");
	    }
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
	active_entities_set(ctx.active_entities, i1, true);

	logd("Created entity: %i\n", i1);
      }else if(snd("model") && editor.selection_kind == SELECTED_ENTITY){
	u32 i1 = index_table_alloc(ctx.models);
	entity_data * entity = index_table_lookup(ctx.entities, editor.selected_index);
	editor.selection_kind = SELECTED_MODEL;
	editor.selected_index = i1;
	entity->model = i1;
      }else if(snd("polygon") && editor.selection_kind == SELECTED_MODEL){

	model_data * entity = index_table_lookup(ctx.models, editor.selected_index);
	u32 pcnt = entity->polygons.count;

	index_table_resize_sequence(ctx.polygon, &(entity->polygons), pcnt + 1);
	editor.selection_kind = SELECTED_MODEL;
	editor.selected_index = entity->polygons.index + entity->polygons.count - 1;
	//logd("creating polygon.. %i %i\n", pcnt, entity->polygons.count);
	//polygon_data * pd = index_table_lookup_sequence(ctx.polygon, entity->polygons);
      }else if(snd("vertex") && editor.selection_kind == SELECTED_POLYGON){
	vec2 p = vec2_zero;
	char buf[20];
	for(u32 i = 0 ; i < array_count(p.data); i++){
	  if(false == copy_nth(command, 2 + i, buf, array_count(buf)))
	    goto skip;
	  sscanf(buf,"%f", p.data + i);
	}
	logd("@ ");vec2_print(p);logd("\n");
	polygon_add_vertex2f(&ctx, editor.selected_index, p);
	u32 idx = index_table_alloc(ctx.polygons_to_delete);
	u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
	ptr[0] = editor.selected_index;

      skip:;
	
      }

      set_simple_graphics_editor_context(control, editor);
      logd("%i %i\n", editor.selection_kind, editor.selected_index);
    }else if(first("set")){
	copy_nth(command, 1, snd_part, array_count(snd_part));
	if(snd("color") && editor.selection_kind == SELECTED_POLYGON) {
	  vec4 p = vec4_zero;
	  char buf[20];
	  for(u32 i = 0 ; i < array_count(p.data); i++){
	    if(false == copy_nth(command, 2 + i, buf, array_count(buf)))
	      goto skip;
	    sscanf(buf,"%f", p.data + i);
	  }
	  logd("Set color: ");vec4_print(p);logd("\n");
	  polygon_color_set(ctx.poly_color, editor.selected_index, p);
	}
      }
    else if(first("select")){
      char id_buffer[100] = {0};
      if(copy_nth(command, 1, snd_part, array_count(snd_part))
	 && copy_nth(command, 2, id_buffer, array_count(id_buffer))){
	u32 i1 = 0;
	sscanf(id_buffer, "%i", &i1);
	logd("SELECT: %s %s id_buffer %i\n", snd_part, id_buffer, id);
	if(snd("entity")){
	  logd("Entity?\n");
	  if(index_table_contains(ctx.entities, i1)){
	    editor.selection_kind = SELECTED_ENTITY;
	    editor.selected_index = i1;
	  }
	  
	}else if(snd("model")){
	  if(index_table_contains(ctx.models, i1)){
	    editor.selection_kind = SELECTED_MODEL;
	    editor.selected_index = i1;
	  }
	}else if(snd("polygon")){
	  if(index_table_contains(ctx.polygon, i1)){
	    editor.selection_kind = SELECTED_POLYGON;
	    editor.selected_index = i1;
	  }
	  
	}else if(snd("vertex")){
	  if(index_table_contains(ctx.vertex, i1)){
	    editor.selection_kind = SELECTED_VERTEX;
	    editor.selected_index = i1;
	  }
	}
      }
      set_simple_graphics_editor_context(control, editor);
	
    }else if(first("remove")){
      char id_buffer[100] = {0};
      if(copy_nth(command, 1, snd_part, array_count(snd_part))){
	u32 i1 = 0;
	if(copy_nth(command, 2, id_buffer, array_count(id_buffer))){
	  sscanf(id_buffer, "%i", &i1);
	}else{
	  i1 = editor.selected_index;
	}
	index_table * table = NULL;
	logd("Remove: %s, %i\n", snd_part, i1);
	if(snd("entity")){
	  table = ctx.entities;
	  
	}else if(snd("model")){
	  table = ctx.models;
	}else if(snd("polygon")){
	  table = ctx.polygon;
	}else if(snd("vertex")){
	  table = ctx.vertex;

	  u32 polygon = vertex_get_polygon(ctx, i1);
	  polygon_data * pd = index_table_lookup(ctx.polygon, polygon);
	  vertex_data * vd = index_table_lookup_sequence(ctx.vertex, pd->vertexes);
	  u32 offset = i1 - pd->vertexes.index;
	  for(u32 i = offset; i < pd->vertexes.count - 1; i++)
	    vd[i] = vd[i + 1];
	  index_table_resize_sequence(ctx.vertex, &(pd->vertexes), pd->vertexes.count - 1);
	  u32 idx = index_table_alloc(ctx.polygons_to_delete);
	  u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
	  ptr[0] = polygon;
	    
	}
	UNUSED(table);
	/*if(table != NULL){
	  if(index_table_contains(table, i1))
	    index_table_remove(table, i1);
	    }*/
	
      }
      set_simple_graphics_editor_context(control, editor);

    }

    else{
      logd("Unkown command!\n");
    }
  }
  logd("COMMAND ENTERED %i %s\n", id, command);

}

#include "console.h"
void simple_graphics_editor_load(u64 id, u64 win_id){
  define_method(id, render_control_method, (method)simple_graphics_editor_render);
  define_method(id, measure_control_method, (method)simple_grid_measure);
  define_method(id, mouse_over_method, (method)simple_grid_mouse_over_func);
  define_method(id, mouse_down_method, (method) simple_grid_mouse_down_func);
  
  u64 console = get_unique_number();
  set_simple_graphics_control(console, id);
  set_focused_element(win_id, console);
  set_console_height(console, 300);
  create_console(console);
  add_control(win_id, console);
  set_margin(console, (thickness){5,5,5,5});
  set_vertical_alignment(console, VALIGN_BOTTOM);
  set_console_history_cnt(console, 100);
  define_method(console, console_command_entered_method, (method)command_entered);

  bool test = true;
  if(test){
    command_entered(console, (char *)"create entity");
    command_entered(console, (char *)"create model");
    command_entered(console, (char *)"create polygon");
    command_entered(console, (char *)"create vertex 0.1 0.1");
    command_entered(console, (char *)"create vertex 0.1 0.3");
    command_entered(console, (char *)"create vertex 0.3 0.1");
    command_entered(console, (char *)"create vertex 0.3 0.3");
    command_entered(console, (char *)"set color 0.3 0.3 0.9 1.0");
    
    command_entered(console, (char *)"create entity");
    command_entered(console, (char *)"create model");
    command_entered(console, (char *)"create polygon");
    command_entered(console, (char *)"create vertex 0.1 0.1");
    command_entered(console, (char *)"create vertex 0.1 -0.3");
    command_entered(console, (char *)"create vertex -0.3 0.1");
    command_entered(console, (char *)"create vertex -0.3 -0.3");
    command_entered(console, (char *)"set color 0.5 0.8 0.3 1.0");

    command_entered(console, (char *)"create entity");
    command_entered(console, (char *)"create model");
    command_entered(console, (char *)"create polygon");
    command_entered(console, (char *)"create vertex 0.1 0.6");
    command_entered(console, (char *)"create vertex 0.1 0.3");
    command_entered(console, (char *)"create vertex -0.3 0.6");
    command_entered(console, (char *)"create vertex -0.3 0.3");
    command_entered(console, (char *)"set color 0.7 0.7 0.3 1.0");
    
  }
  
}


bool simple_graphics_editor_test(){
  u64 item = get_unique_number();
  u64 win_id = intern_string("board");
  simple_grid_renderer_create(item);
  simple_graphics_editor_load(item, win_id);
  u64 console_id = 0;
  {
    u64 keys[100];
    u64 values[100];
    u64 idx = 0;
    iter_all_simple_graphics_control(keys, values, array_count(keys), &idx);
    for(u64 i = 0; i < array_count(keys); i++){
      if(values[i] == item){
	console_id = keys[i];
	break;
      }
    }
  }
  TEST_ASSERT(console_id != 0);
				     
  command_entered(console_id, (char *)"create entity");
  command_entered(console_id, (char *)"create model");
  command_entered(console_id, (char *)"create polygon");
  command_entered(console_id, (char *)"create vertex 0.1 0.1");
  command_entered(console_id, (char *)"create vertex 0.1 1");
  command_entered(console_id, (char *)"create vertex 1 0.1");
  command_entered(console_id, (char *)"create vertex 1 1");
  return TEST_SUCCESS;
}


