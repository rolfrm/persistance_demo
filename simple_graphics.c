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
    model_data * m2 = lookup_index_table(models, index);
    index = m2->index;

    switch(m2->type){
    case KIND_MODEL:
      print_model_data(m2->index, models, polygon, vertex);
      break;
    case KIND_POLYGON:
      {
	logd("POLYGON:\n");
	polygon_data * pd = lookup_index_table(polygon, index);
	while(index != 0){
	  if(pd->vertex != 0){
	    vertex_data * vd = lookup_index_table(vertex, pd->vertex);
	    logd("  Vertex: %p ", vd->color);
	    vec2_print(vd->position);
	    logd("\n");
	  }
	  
	  index = pd->next_index;
	  if(index != 0)
	    pd = lookup_index_table(polygon, index);
	}
      }
	
    }
    
  }
}

typedef struct{
  vec3 position;
  u32 model;
}entity_data;

void test_simple_graphics(){
  
  index_table * models = create_index_table("simple/models", sizeof(model_data));
  index_table * polygon = create_index_table("simple/polygon", sizeof(polygon_data));
  index_table * vertex = create_index_table("simple/vertex", sizeof(vertex_data));
  index_table * entities = create_index_table("simple/entities", sizeof(entity_data));  
  logd("%i \n", models->ptr->size);
  u32 i1 = alloc_index_table(models);
  
  logd("%i %p %p\n", i1, polygon->ptr->ptr, vertex->ptr->ptr);
  model_data * m1 = lookup_index_table(models, i1);
  m1->type = KIND_POLYGON;
  {

    u32 p1 = alloc_index_table(polygon);
    u32 p2 = alloc_index_table(polygon);
    u32 p3 = alloc_index_table(polygon);
    alloc_index_table(polygon);
    alloc_index_table(polygon);
    alloc_index_table(polygon);
    
    u32 t1 = alloc_index_table(vertex);
    u32 t2 = alloc_index_table(vertex);
    u32 t3 = alloc_index_table(vertex);
    logd("%p %p\n", polygon->ptr->ptr, vertex->ptr->ptr);    

    m1->index = p1;
    vertex_data * v = lookup_index_table(vertex, t1);
    v->color = 0xFFFFFFFF;
    v->position = vec2_new(1,0);
    v = lookup_index_table(vertex, t2);
    v->color = 0xFFFFFFFF;
    v->position = vec2_new(1,1);
    v = lookup_index_table(vertex, t3);
    v->color = 0xFFFFFFFF;
    v->position = vec2_new(0,1);
    polygon_data * pd = lookup_index_table(polygon, p1);
    pd->next_index = p2;
    pd->vertex = t1;
    pd = lookup_index_table(polygon, p2);
    pd->next_index = p3;
    pd->vertex = t2;

    pd = lookup_index_table(polygon, p3);
    pd->next_index = 0;
    pd->vertex = t3;
  }
  u32 player = alloc_index_table(entities);
  entity_data * ed = lookup_index_table(entities, player);
  ed->position = vec3_new(0,0,0);
  ed->model = i1;

  print_model_data(i1, models, polygon, vertex);
}
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

  model_data * model = lookup_index_table(gd.models, model_id);
  switch(model->type){
  case KIND_POLYGON:{
    loaded_polygon_data loaded;
    
    if(false == try_get_loaded_polygon(model->index, &loaded)) {
      
      u32 index = model->index;
      u32 count = 0;
      for(u32 index = model->index; index != 0; index = ((polygon_data *)lookup_index_table(gd.polygon, index))->next_index)
	count += 1;
      
      vec2 positions[count];
      vec2 * p = &positions[0];
      while(index != 0){
	polygon_data * pd = lookup_index_table(gd.polygon, index);
	index = pd->next_index;
	vertex_data * vd = lookup_index_table(gd.vertex, pd->vertex);
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
      entity_data * ed = lookup_index_table(gd.entities, entity);
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
