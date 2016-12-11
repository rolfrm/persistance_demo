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
  u32 gpu_id;

}gpu_polygon;

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
  vec3 position;
  u32 model;
}entity_data;

typedef struct{
  u32 gl_ref;
  u32 count;
}loaded_polygon_data;




/*
typedef index_table polygon_table;
typedef u32 polygon_id;

polygon_id polygon_create(graphics_context * ctx);
u32 polygon_add_vertex2f(graphics_context * ctx, polygon_id polygon, vec3 offset);
void polygon_set_color(graphics_context * ctx, polygon_id polygon, vec4 color);
void polygon_read(graphics_context * ctx, polygon_id polygon, vec3 ** offsets, u64 count);
void polygon_render(graphics_context * ctx, polygon_id polygon, mat3 tform, vec4 color);

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
  polygon_color_table * poly_color;
  active_entities_table * active_entities;
}graphics_context;

typedef u32 polygon_id;

polygon_id polygon_create(graphics_context * ctx){
  return index_table_alloc(ctx->polygon);
}

void polygon_add_vertex2f(graphics_context * ctx, polygon_id polygon, vec2 offset){
  u32 t1 = index_table_alloc(ctx->vertex);
  vertex_data * v = index_table_lookup(ctx->vertex, t1);
  v->position = offset;
  
  polygon_data * pd = index_table_lookup(ctx->polygon, polygon);
  if(pd->vertex == 0){
    pd->vertex = t1;
    return;
  }
  while(pd->next_index != 0){
    polygon = pd->next_index;
    pd = index_table_lookup(ctx->polygon, polygon);
  }
  logd("pd: %i %i\n", pd->next_index, polygon);
  
  polygon_id p1 = polygon_create(ctx);
  pd->next_index = p1;
  pd = index_table_lookup(ctx->polygon, p1);
  pd->vertex = t1;
}

void graphics_context_load(graphics_context * ctx){
  ctx->models = index_table_create(NULL/*"simple/models"*/, sizeof(model_data));
  ctx->polygon = index_table_create(NULL/*"simple/polygon"*/, sizeof(polygon_data));
  ctx->vertex = index_table_create(NULL/*"simple/vertex"*/, sizeof(vertex_data));
  ctx->entities = index_table_create(NULL/*"simple/entities"*/, sizeof(entity_data));
  ctx->gpu_poly = loaded_polygon_table_create(NULL);
  ctx->poly_color = polygon_color_table_create(NULL);
  ctx->active_entities = active_entities_table_create(NULL);
}

void simple_graphics_load_test(graphics_context * ctx){
  logd("Load test\n");
  index_table * models = ctx->models;
  index_table * entities = ctx->entities;
  ctx->active_entities = active_entities_table_create(NULL);
  u32 i1 = index_table_alloc(models);
  
  model_data * m1 = index_table_lookup(models, i1);
  m1->type = KIND_POLYGON;
  {
    polygon_id p1 = polygon_create(ctx);
    polygon_add_vertex2f(ctx, p1, vec2_new(1,0));
    polygon_add_vertex2f(ctx, p1, vec2_new(0.8,0.8));
    polygon_add_vertex2f(ctx, p1, vec2_new(0,1));
    polygon_add_vertex2f(ctx, p1, vec2_new(0.2,0.2));
    polygon_add_vertex2f(ctx, p1, vec2_new(-0.5,-0.5));
    m1->index = p1;
  }
  u32 player = index_table_alloc(entities);
  entity_data * ed = index_table_lookup(entities, player);
  ed->position = vec3_new(0,0,0);
  ed->model = i1;

  active_entities_set(ctx->active_entities, player, true);
}


CREATE_TABLE_DECL2(graphics_context, u64, graphics_context);
CREATE_TABLE2(graphics_context, u64, graphics_context);
u32 simple_grid_polygon_load(vec2 * vertexes, u32 count){
  u32 vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes[0]) * count, vertexes, GL_STATIC_DRAW);
  return vbo;
}

void simple_grid_render_gl(loaded_polygon_data * loaded, mat4 camera){
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
  }
  
  glUseProgram(shader);
  glUniformMatrix4fv(camera_loc,1,false, &(camera.data[0][0]));
  glUniform4f(color_loc, 154.0 / 255.0, 36.0 / 255.0, 31.0 / 255.0, 1);
  glEnableVertexAttribArray(vertex_loc);
  glVertexAttribPointer(vertex_loc, 2, GL_FLOAT, false, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, loaded->gl_ref);
  glDrawArrays(GL_TRIANGLE_FAN, 0, loaded->count);
}


void simple_grid_render_internal(u32 model_id, vec3 offset, graphics_context gd){
  
  model_data * model = index_table_lookup(gd.models, model_id);
  switch(model->type){
  case KIND_POLYGON:{
    loaded_polygon_data loaded;
    if(false == loaded_polygon_try_get(gd.gpu_poly, model->index, &loaded)) {
      u32 index = model->index;
      u32 count = 0;
      for(u32 index = model->index; index != 0; index = ((polygon_data *)index_table_lookup(gd.polygon, index))->next_index)
	count += 1;
      
      vec2 positions[count];
      vec2 * p = &positions[0];
      while(index != 0){
	polygon_data * pd = index_table_lookup(gd.polygon, index);
	index = pd->next_index;
	vertex_data * vd = index_table_lookup(gd.vertex, pd->vertex);
	*p = vd->position;
	p += 1;
      }
      ASSERT(count > 0);
      loaded.gl_ref = simple_grid_polygon_load(positions, count);
      ASSERT(loaded.gl_ref > 0);
      loaded.count = count;
      loaded_polygon_set(gd.gpu_poly, model->index, loaded);
    }
    simple_grid_render_gl(&loaded, mat4_scaled(0.5, 0.5, 0.5));
  }
  default:
    break;

  }
  UNUSED(model);UNUSED(offset);
}


void simple_grid_render(u64 id){
  graphics_context gd = get_graphics_context(id);
  u32 entities[10];
  bool unused[10];
  u64 idx = 0;
  u64 cnt = 0;
  while(0 != (cnt = active_entities_iter_all(gd.active_entities, entities,unused, array_count(unused), &idx))){
    for(u64 i = 0; i < cnt; i++){
      u32 entity = entities[i];
      entity_data * ed = index_table_lookup(gd.entities, entity);
      if(ed->model != 0)
	simple_grid_render_internal(ed->model, ed->position, gd);
    }
  }
}

void simple_grid_renderer_create(u64 id){
  graphics_context gd;
  gd.gpu_poly = loaded_polygon_table_create(NULL);
  graphics_context_load(&gd);
  simple_graphics_load_test(&gd);
  logd("Created gd %i\n", gd.models->area->size);
  set_graphics_context(id, gd);
}

void simple_grid_measure(u64 id, vec2 * size){
  UNUSED(id);
  *size = vec2_new(0, 0);
  
}


void simple_grid_initialize(u64 id){
  define_method(id, render_control_method, (method)simple_grid_render);
  define_method(id, measure_control_method, (method)simple_grid_measure);
}
