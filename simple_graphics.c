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

CREATE_TABLE_DECL2(active_entities, u32, bool);
CREATE_TABLE_NP(active_entities, u32, bool);
CREATE_TABLE_DECL2(loaded_polygon, u32, loaded_polygon_data);
CREATE_TABLE_NP(loaded_polygon, u32, loaded_polygon_data);

typedef struct{
  index_table * entities;
  index_table * models;
  index_table * polygon;
  index_table * vertex;
  loaded_polygon_table * gpu_poly;
}graphics_data;
void simple_graphics_load_test(graphics_data * result){
  
  index_table * models = index_table_create("simple/models", sizeof(model_data));
  index_table * polygon = index_table_create("simple/polygon", sizeof(polygon_data));
  index_table * vertex = index_table_create("simple/vertex", sizeof(vertex_data));
  index_table * entities = index_table_create("simple/entities", sizeof(entity_data));  
  u32 i1 = index_table_alloc(models);
  
  model_data * m1 = index_table_lookup(models, i1);
  m1->type = KIND_POLYGON;
  {

    u32 p1 = index_table_alloc(polygon);
    index_table_remove(polygon, p1);
    p1 = index_table_alloc(polygon);
    u32 p2 = index_table_alloc(polygon);
    u32 p3 = index_table_alloc(polygon);
    index_table_remove(polygon, p2);
    index_table_remove(polygon, p3);
    p2 = index_table_alloc(polygon);
    p3 = index_table_alloc(polygon);
    p1 = index_table_alloc(polygon);
    index_table_alloc(polygon);
    index_table_alloc(polygon);
    index_table_alloc(polygon);
    
    u32 t1 = index_table_alloc(vertex);
    u32 t2 = index_table_alloc(vertex);
    u32 t3 = index_table_alloc(vertex);

    m1->index = p1;
    vertex_data * v = index_table_lookup(vertex, t1);
    v->color = 0xFFFFFFFF;
    v->position = vec2_new(1,0);
    v = index_table_lookup(vertex, t2);
    v->color = 0xFFFFFFFF;
    v->position = vec2_new(1,1);
    v = index_table_lookup(vertex, t3);
    v->color = 0xFFFFFFFF;
    v->position = vec2_new(0,1);
    polygon_data * pd = index_table_lookup(polygon, p1);
    pd->next_index = p2;
    pd->vertex = t1;
    pd = index_table_lookup(polygon, p2);
    pd->next_index = p3;
    pd->vertex = t2;

    pd = index_table_lookup(polygon, p3);
    pd->next_index = 0;
    pd->vertex = t3;
  }
  u32 player = index_table_alloc(entities);
  entity_data * ed = index_table_lookup(entities, player);
  ed->position = vec3_new(0,0,0);
  ed->model = i1;

  //print_model_data(i1, models, polygon, vertex);
  result->entities = entities;
  result->models = models;
  result->vertex = vertex;
  result->polygon = polygon;
}


void simple_graphics_test(){
  graphics_data gd;
  simple_graphics_load_test(&gd);
  logd("DONE\n");

}
CREATE_TABLE_DECL2(graphics_data, u64, graphics_data);
CREATE_TABLE2(graphics_data, u64, graphics_data);
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
  glUniform4f(color_loc, 1, 1, 1, 1);
  glEnableVertexAttribArray(vertex_loc);
  glVertexAttribPointer(vertex_loc, 2, GL_FLOAT, false, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, loaded->gl_ref);
  glDrawArrays(GL_POINTS, 0, loaded->count);
}


void simple_grid_render_internal(u32 model_id, vec3 offset, graphics_data gd){

  model_data * model = index_table_lookup(gd.models, model_id);
  switch(model->type){
  case KIND_POLYGON:{
    loaded_polygon_data loaded;
    
    if(false == try_get_loaded_polygon(model->index, &loaded)) {
      
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
      loaded.gl_ref = simple_grid_polygon_load(positions, count);
      loaded.count = count;
      set_loaded_polygon(model->index, loaded);
    }
    

  }
  default:
    break;

  }
  UNUSED(model);UNUSED(offset);
}

void simple_grid_render(u64 id){
  graphics_data gd = get_graphics_data(id);
  u32 entities[10];
  bool unused[10];
  u64 idx = 0;
  u64 cnt = 0;
  while(0 != (cnt = iter_all_active_entities(entities,unused, array_count(unused), &idx))){
    for(u64 i = 0; i < cnt; i++){
      u32 entity = entities[i];
      entity_data * ed = index_table_lookup(gd.entities, entity);
      if(ed->model != 0)
	simple_grid_render_internal(ed->model, ed->position, gd);
    }
  }
}


graphics_data simple_grid_renderer_init(){
  graphics_data gd;
  gd.gpu_poly = loaded_polygon_table_create(NULL);
  return gd;

}
