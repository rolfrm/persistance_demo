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
#include "console.h"
#include "simple_graphics.h"

CREATE_TABLE2(desc_text, u64, name_data);
CREATE_TABLE2(entity_type, u64, ENTITY_TYPE);

void set_desc_text2(u32 idx, u32 group, const char * str){
  u64 id = ((u64)idx) | ( ((u64)group) << 32);
  name_data d = {0};
  memcpy(d.data, str, strlen(str));
  set_desc_text(id, d);
  logd("%p\n", id);
}

bool get_desc_text2(u32 idx, u32 group, char * buf, u32 bufsize){
  u64 id = ((u64)idx) | ( ((u64)group) << 32);
  name_data d;
  if(try_get_desc_text(id, &d)){
    memcpy(buf, d.data, bufsize);
    buf[bufsize] = 0;
    return true;
  }else{
    buf[0] = 0;
    return false;
  }
}

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

CREATE_TABLE_DECL2(game_window, u64, u64);
CREATE_TABLE2(game_window, u64, u64);
CREATE_TABLE_NP(active_entities, u32, bool);
CREATE_TABLE_NP(loaded_polygon, u32, loaded_polygon_data);
CREATE_TABLE2(polygon_color, u32, vec4);
CREATE_TABLE_DECL2(gravity_affects, u64, bool);
CREATE_TABLE2(gravity_affects, u64, bool);
CREATE_TABLE2(current_impulse, u64, vec3);
CREATE_MULTI_TABLE2(entity_2_collisions, u32, u32);

CREATE_TABLE2(entity_speed, u32, f32);
CREATE_TABLE2(ghost_material, u32, bool);

typedef u32 polygon_id;
polygon_id polygon_create(graphics_context * ctx);


void graphics_context_reload_polygon(graphics_context ctx, u32 polygon){
  polygon_data * pd = index_table_lookup(ctx.polygon, polygon);
  loaded_polygon_data loaded;
  if(loaded_polygon_try_get(ctx.gpu_poly, pd->material, &loaded)){
    u32 idx = index_table_alloc(ctx.polygons_to_delete);
    u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
    ptr[0] = pd->material;
  }
}

typedef struct{
  union{
    char name[40];
    char reserved[100];
  };
}module_name;
int load_module(graphics_context * ctx, char * name);
void graphics_context_load(graphics_context * ctx){
  memset(ctx, 0, sizeof(*ctx));
  ctx->models = index_table_create("simple/models", sizeof(model_data));
  ctx->polygon = index_table_create("simple/polygon", sizeof(polygon_data));

  ctx->vertex = index_table_create("simple/vertex", sizeof(vertex_data));
  ctx->entities = index_table_create("simple/entities", sizeof(entity_data));
  ctx->loaded_modules = index_table_create("simple/loaded_modules", sizeof(module_name));
  ctx->polygons_to_delete = index_table_create(NULL, sizeof(u32));
  ctx->gpu_poly = loaded_polygon_table_create(NULL);
  ctx->poly_color = polygon_color_table_create("simple/polygon_color");
  ctx->active_entities = active_entities_table_create("simple/active_entities");
  ctx->entity_speed = entity_speed_table_create("simple/entity_speed");
  { // find cursor.
    u64 cnt = 0;
    polygon_data * pd = index_table_all(ctx->polygon, &cnt);
    polygon_id pid = 0;
    for(u64 i = 0; i < cnt; i++){
      char nd[100];
      if(get_desc_text2(pd[i].material,SELECTED_POLYGON, nd, sizeof(nd))){
	if(strcmp(nd, "pointer_poly") == 0){
	  pid = i + 1;
	  break;
	}
      }
    }
    if(pid == 0){
      pid = polygon_create(ctx);
      ctx->pointer_index = pid;
      polygon_add_vertex2f(ctx, pid, vec2_new(0, 0));
      polygon_add_vertex2f(ctx, pid, vec2_new(0.2, 0));
      polygon_add_vertex2f(ctx, pid, vec2_new(0.2, 0.2));
      set_desc_text2(SELECTED_POLYGON, pid, "pointer_poly");
      pd = index_table_lookup(ctx->polygon, pid);
      pd->material = get_unique_number();
    }
    if(pd->vertexes.count < 2){
      polygon_add_vertex2f(ctx, pid, vec2_new(0.2, 0));
      polygon_add_vertex2f(ctx, pid, vec2_new(0.2, 0.2));
    }

    vertex_data * vd = index_table_lookup_sequence(ctx->vertex, pd->vertexes);
    vd[0].position = vec2_new(0, 0);
    vd[1].position = vec2_new(0.01, 0.03);
    vd[2].position = vec2_new(0.03, 0.01);
  }
  
  ctx->collision_table = index_table_create(NULL, sizeof(collision_data));
  ctx->collisions_2_table = entity_2_collisions_table_create(NULL);
  ctx->interactions = simple_game_interactions_table_create(NULL);
  ctx->game_update_functions = simple_game_update_table_create(NULL);
  ctx->editor_functions = simple_game_editor_fcn_table_create(NULL);
  u64 cnt = 0;
  module_name * modules = index_table_all(ctx->loaded_modules, &cnt);
  for(u32 i = 0; i < cnt; i++)
    load_module(ctx, modules[i].name);
  ctx->ghost_table = ghost_material_table_create("simple/ghost_material");
  
}
CREATE_TABLE_DECL2(current_loaded_modules, u32, u32);
CREATE_TABLE_NP(current_loaded_modules, u32, u32);
#include<dlfcn.h>
int load_module(graphics_context * ctx, char * name){
  u64 cnt = 0;
  module_name * modules = index_table_all(ctx->loaded_modules, &cnt);
  int found = -1;
  for(u32 i = 0; i < cnt; i++){
    if(strcmp(modules[i].name, name) == 0){
      u32 c = 0;
      if(try_get_current_loaded_modules(i, &c)){
	logd("Module is already loaded..\n");
	return 0;
      }
      found = i;
      break;
    }
  }
  if(found == -1){
    u32 idx  = index_table_alloc(ctx->loaded_modules);
    module_name * mod = index_table_lookup(ctx->loaded_modules, idx);
    memset(mod->name, 0, sizeof(mod->name));
    strcpy(mod->name, name);
    
    set_current_loaded_modules(idx, 1);
  }else{
    set_current_loaded_modules(found + 1, 1);
  }
  
  logd("Loading module '%s'\n", name);
  UNUSED(ctx);
  void * module = dlopen(name, RTLD_NOW);
  if(module == NULL){
    loge("unable to load module '%s' %s\n", name, dlerror());
    return -1;
  }

  void (* init_module)(graphics_context * ctx) = dlsym(module, "init_module");
  if(init_module == NULL){
    loge("Unable to get 'init_module' from module '%s'\n", name);
    return -1;
  }
  init_module(ctx);
  return 0;
}

CREATE_TABLE_NP(simple_game_interactions, u32, interact_fcn);
CREATE_TABLE_NP(simple_game_update, u32, simple_game_update_fcn);
CREATE_TABLE_NP(simple_game_editor_fcn, u32, simple_graphics_editor_fcn);
void graphics_context_load_interaction(graphics_context * ctx, interact_fcn f, u32 id){
  simple_game_interactions_set(ctx->interactions, id, f);
}

void graphics_context_load_update(graphics_context * ctx, simple_game_update_fcn f, u32 id){
  simple_game_update_set(ctx->game_update_functions, id, f);
}

void simple_game_editor_load_func(graphics_context * ctx, simple_graphics_editor_fcn f, u32 id){
  simple_game_editor_fcn_set(ctx->editor_functions, id, f);
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
    if(offset <= vertex && vertex < offset + count){
      return i + 1;
    }
  }
  return 0;
}

u32 polygon_get_model(graphics_context ctx, u32 polygon){
  u64 modelcnt = 0;
  model_data * md = index_table_all(ctx.models, &modelcnt);
  for(u64 i = 0; i < modelcnt; i++){
    u32 offset = md[i].polygons.index;
    u32 count = md[i].polygons.count;
    if(offset <= polygon && polygon < offset + count){
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
  return vbo;
}

typedef struct{
  u32 shader;
  u32 camera_loc;
  u32 color_loc;
  u32 vertex_loc;
  u32 depth_loc;
}simple_grid_shader;

simple_grid_shader simple_grid_shader_get(){
  static bool initialized = false;
  static u32 shader = 0;
  static u32 camera_loc, color_loc, vertex_loc, depth_loc;
  
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
    depth_loc = glGetUniformLocation(shader, "depth");
    logd("Loading shader..\n");
  }
  simple_grid_shader _shader = {
    .shader = shader,
    .camera_loc = camera_loc,
    .color_loc = color_loc,
    .vertex_loc = vertex_loc,
    .depth_loc = depth_loc
  };
  return _shader;
}

vec2 graphics_context_pixel_to_screen(const graphics_context ctx, vec2 pixel_coords){
  ASSERT(fabs(ctx.render_size.x) >=0.001);
  vec2 v = vec2_sub(vec2_scale(vec2_div(pixel_coords, ctx.render_size), 2.0), vec2_one);
  v.y = -v.y;
 
  return v;
}

loaded_polygon_data simple_grid_load_polygon(const graphics_context ctx, u32 polygon_id){
  loaded_polygon_data loaded = {0};
  ASSERT(polygon_id != 0);
  polygon_data * pd = index_table_lookup(ctx.polygon, polygon_id);
  if(pd->vertexes.index == 0)
    return loaded;
  if(pd->material == 0)
    return loaded;
  if(false == loaded_polygon_try_get(ctx.gpu_poly, pd->material, &loaded)) {
    u32 count = pd->vertexes.count;
    
    if(count == 0)
      return loaded;
    vec2 positions[count];
    float max_depth = -100;
    vertex_data * vd = index_table_lookup_sequence(ctx.vertex, pd->vertexes);
    for(u32 i = 0; i < count; i++){
      positions[i] = vd[i].position;
      max_depth = MAX(vd[i].position.y, max_depth);
      
    }
    
    ASSERT(count > 0);
    loaded.gl_ref = simple_grid_polygon_load(positions, count);
    loaded.depth = max_depth;
    ASSERT(loaded.gl_ref > 0);
    loaded.count = count;
    loaded.depth = max_depth;
    loaded_polygon_set(ctx.gpu_poly, pd->material, loaded);
  }
  return loaded;
}

CREATE_TABLE_DECL2(grid_poly, u32, u32);
CREATE_TABLE2(grid_poly, u32, u32);


void simple_grid_render_gl(const graphics_context ctx, u32 polygon_id, mat4 camera, bool draw_points, float depth){
  loaded_polygon_data loaded = simple_grid_load_polygon(ctx, polygon_id);
  if(loaded.gl_ref == 0)
    return;
  polygon_data * pd = index_table_lookup(ctx.polygon, polygon_id);
  glEnable( GL_BLEND );
  if(pd->physical_height != 0.0f){

    depth += loaded.depth;
  }
  //logd("%i %f\n", polygon_id, depth);  
  glBindBuffer(GL_ARRAY_BUFFER, loaded.gl_ref);  
  simple_grid_shader shader = simple_grid_shader_get();
  glFrontFace(GL_CW);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  
  vec4 color = vec4_zero;
  polygon_color_try_get(ctx.poly_color, pd->material, &color);
  if(color.w <= 0.0f)
    return;
  if(depth > -100){
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
  }else{
    glDisable(GL_DEPTH_TEST);
  }
  glUseProgram(shader.shader);
  glUniformMatrix4fv(shader.camera_loc,1,false, &(camera.data[0][0]));
  glUniform4f(shader.color_loc, color.x, color.y, color.z, color.w);
  glUniform1f(shader.depth_loc, (depth + 10));
  glEnableVertexAttribArray(shader.vertex_loc);
  glVertexAttribPointer(shader.vertex_loc, 2, GL_FLOAT, false, 0, 0);
  
  glDrawArrays(GL_TRIANGLE_STRIP, 0, loaded.count);
  if(draw_points){
    glPointSize(3);
    glDrawArrays(GL_POINTS, 0, loaded.count);
  }
  glDisableVertexAttribArray(shader.vertex_loc);
  glDisable(GL_CULL_FACE);
  glDisable( GL_DEPTH_TEST );
}



CREATE_TABLE_DECL2(simple_graphics_editor_context, u64, editor_context);
CREATE_TABLE2(simple_graphics_editor_context, u64, editor_context);

void simple_grid_mouse_over_func(u64 grid_id, double x, double y, u64 method){
  graphics_context ctx = get_graphics_context(grid_id);
  editor_context editor = get_simple_graphics_editor_context(grid_id);
  vec2 last_position = ctx.pointer_position;
  ctx.pointer_position = vec2_new(x, y);
  set_graphics_context(grid_id, ctx);
  if(method != 0){
    auto on_mouse_over = get_method(grid_id, method);
    on_mouse_over(grid_id, x, y, 0);
  }else{
    if(editor.is_grabbed != false){
      
      vec2 d = vec2_sub(graphics_context_pixel_to_screen(ctx, ctx.pointer_position), graphics_context_pixel_to_screen(ctx, last_position));
      if(editor.zoom > 0)
      d = vec2_scale(d, 1.0/editor.zoom);
      editor.offset = vec2_add(editor.offset, d);
      set_simple_graphics_editor_context(grid_id, editor);
    }
  }
}


CREATE_TABLE_DECL2(simple_graphics_editor_grid_width, u64, f32);
CREATE_TABLE2(simple_graphics_editor_grid_width, u64, f32);


void simple_grid_mouse_down_func(u64 grid_id, double x, double y, u64 method){
  UNUSED(method);
  if(mouse_button_action == mouse_button_repeat) return;

  graphics_context ctx = get_graphics_context(grid_id);
  editor_context editor = get_simple_graphics_editor_context(grid_id);
  vec2 p = graphics_context_pixel_to_screen(ctx, vec2_new(x, y));
  p = vec2_scale(p, 1.0 / editor.zoom);
  p = vec2_sub(p, editor.offset);
  //mat4 editor_tform = mat4_translate(editor.offset.x, editor.offset.y, 0);
  //editor_tform = mat4_mul(mat4_scaled(editor.zoom, editor.zoom, editor.zoom), editor_tform);
  
  if(editor.snap_to_grid){
    float grid_width = 0.1;
    if(!try_get_simple_graphics_editor_grid_width(grid_id, &grid_width))
      grid_width = 0.1;
    //mat4 inv = mat4_invert(editor_tform);
    //vec4 p = mat4_mul_vec4(inv, vec4_new(offset.x, offset.y, 0, 1));
    if(editor.focused_item_kind == SELECTED_ENTITY){
      entity_data * ed2 = index_table_lookup(ctx.entities, editor.focused_item);
      p.x = p.x + ed2->position.x;
      p.y = p.y + ed2->position.y;
      p.x = round(p.x / grid_width) * grid_width;
      p.y = round(p.y / grid_width) * grid_width;
      p.x = p.x - ed2->position.x;
      p.y = p.y - ed2->position.y;
    
    }
    
  }
  
  if(mouse_button_button == mouse_button_left){
    if(mouse_button_action != mouse_button_press)
      return;
    if(editor.selection_kind == SELECTED_VERTEX){
      vertex_data * vd = index_table_lookup(ctx.vertex, editor.selected_index);
      vd->position = p;
      u32 polygon = vertex_get_polygon(ctx, editor.selected_index);
      graphics_context_reload_polygon(ctx, polygon);
      
    }else if(editor.selection_kind == SELECTED_POLYGON){
      polygon_add_vertex2f(&ctx, editor.selected_index, p);
      graphics_context_reload_polygon(ctx, editor.selected_index);
    }else if(editor.selection_kind == SELECTED_ENTITY && editor.selected_index != 0){
      entity_data * ed = index_table_lookup(ctx.entities, editor.selected_index);
      ed->position = vec3_new(ed->position.x + p.x, ed->position.y, ed->position.z + p.y);

    }
  } else if(mouse_button_button == mouse_button_right){
    editor.is_grabbed = mouse_button_action == mouse_button_press;
    u64 win = get_game_window(grid_id);
    if(editor.is_grabbed){
      gui_acquire_mouse_capture(win, grid_id);
      gui_set_cursor_mode(win, gui_window_cursor_disabled);
    }else{
      gui_release_mouse_capture(win, grid_id);
      gui_set_cursor_mode(win, gui_window_cursor_normal);
    }
    set_simple_graphics_editor_context(grid_id, editor);
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

  u32 entities[10];
  bool unused[10];
  u64 idx = 0;
  u64 cnt = 0;

  editor_context ed = get_simple_graphics_editor_context(id);
  mat4 editor_tform = mat4_translate(ed.offset.x, ed.offset.y, 0);
  if(ed.focused_item_kind == SELECTED_ENTITY){
    if(ed.focused_item != 0){
      entity_data * ed2 = index_table_lookup(gd.entities, ed.focused_item);
      if(ed2 != NULL){
	vec3 p = ed2->position;;
	editor_tform = mat4_mul(editor_tform, mat4_translate(-p.x, -p.y , -p.z));
      }
    }
  }else if(false && ed.focused_item_kind == SELECTED_MODEL){
    model_data * model = index_table_lookup(gd.models, ed.focused_item);
    for(u32 j = 0; j < model->polygons.count; j++){
      u32 index = j + model->polygons.index;
      simple_grid_render_gl(gd, index, mat4_identity(), true, 0.5);
    }
  }

  editor_tform = mat4_mul(mat4_scaled(ed.zoom, ed.zoom, ed.zoom), editor_tform);

  mat4 cammat = mat4_identity();
  cammat.m22 = 0;
  cammat.m21 = 1;
  
  while(0 != (cnt = active_entities_iter_all(gd.active_entities, entities,unused, array_count(unused), &idx))){

    for(u64 i = 0; i < cnt; i++){
      u32 entity = entities[i];
      entity_data * ed = index_table_lookup(gd.entities, entity);
      vec3 p = ed->position;
      mat4 tform = mat4_translate(p.x, p.y, p.z);
      tform = mat4_mul(cammat, mat4_mul(editor_tform, tform));
      if(ed->model != 0){
	model_data * model = index_table_lookup(gd.models, ed->model);
	for(u32 j = 0; j < model->polygons.count; j++){
	  u32 index = j + model->polygons.index;
	  simple_grid_render_gl(gd, index, tform, true, p.z);
	}
      }
    }
  }
  
  {
    float grid_width = 0.1;
    if(!try_get_simple_graphics_editor_grid_width(id, &grid_width))
      grid_width = 0.1;
    polygon_data * pd = index_table_lookup(gd.polygon, gd.pointer_index);
    if(pd->material == 0)
      pd->material = get_unique_number();
    polygon_color_set(gd.poly_color, pd->material, vec4_new(0, 0, 0, 1));
    vec2 offset = graphics_context_pixel_to_screen(gd, gd.pointer_position);
    if(ed.snap_to_grid){
      mat4 inv = mat4_invert(editor_tform);
      vec4 p = mat4_mul_vec4(inv, vec4_new(offset.x, offset.y, 0, 1));
      p.x = round(p.x / grid_width) * grid_width;
      p.y = round(p.y / grid_width) * grid_width;
      offset = mat4_mul_vec4(editor_tform, p).xyz.xy;
    }
    auto tf = mat4_translate(offset.x, offset.y, 0);
    simple_grid_render_gl(gd, gd.pointer_index, tf, false, -1000);
    polygon_color_set(gd.poly_color, pd->material, vec4_new(1, 1, 1, 1));
    simple_grid_render_gl(gd, gd.pointer_index,
			  mat4_mul(tf,  mat4_scaled(0.5, 0.5, 0.5)), false, -1000); 
  }

  { // Here grid lines are rendered
    glDisable(GL_DEPTH_TEST);
    //mat4 m = mat4_identity();
    u64 polyid = intern_string("__grid_polygon__");
    float grid_width = 0.1;
    if(!try_get_simple_graphics_editor_grid_width(id, &grid_width))
      grid_width = 0.1;
    if(true){
      
      u32 poly = get_grid_poly(polyid);
      if(poly == 0){
	set_grid_poly(polyid, poly = polygon_create(&gd));
	polygon_add_vertex2f(&gd, poly, vec2_new(-1.0, 0.0));
      }
      polygon_data * pd = index_table_lookup(gd.polygon, poly);
      if(pd->material == 0)
	pd->material = get_unique_number();
      
      if(pd->vertexes.count == 1)
	polygon_add_vertex2f(&gd, poly, vec2_new(1.0, 0.0));
      mat4 inv = mat4_invert(editor_tform);
      vec4 vout = mat4_mul_vec4(inv, vec4_new(-1,-1,0,1));
      vec4 vout2 = mat4_mul_vec4(inv, vec4_new(1,1,0,1));
      vec3 v1 = vec3_scale(vout.xyz, 1.0 / grid_width);
      vec3 v2 = vec3_scale(vout2.xyz, 1.0 / grid_width);
      if(v1.x > v2.x) SWAP(v1.x, v2.x);
      if(v1.y > v2.y) SWAP(v1.y, v2.y);
      float px = v1.x;
      float py = v1.y;
      v1.x = floor(v1.x);
      v1.y = floor(v1.y);
      v2.x = ceil(v2.x);
      v2.y = ceil(v2.y);
      if(v2.x - v1.x < 100){
	for(;v1.y < v2.y; v1.y += 1){
	  vec4 v = vec4_new(px * grid_width, v1.y * grid_width,0,1);
	  v = mat4_mul_vec4(editor_tform, v);
	  mat4 cam = mat4_translate(v.x + 1, v.y, v.z);
	  auto loaded = simple_grid_load_polygon(gd, poly);
	  glBindBuffer(GL_ARRAY_BUFFER, loaded.gl_ref);
	  simple_grid_shader shader = simple_grid_shader_get();
	  glUseProgram(shader.shader);
	
	  glUniformMatrix4fv(shader.camera_loc,1,false, &(cam.data[0][0]));
	  glUniform4f(shader.color_loc, 0, 0, 0, 0.2);
	  glEnableVertexAttribArray(shader.vertex_loc);
	  glVertexAttribPointer(shader.vertex_loc, 2, GL_FLOAT, false, 0, 0);
	  glDrawArrays(GL_LINE_LOOP, 0, loaded.count);
	  glDisableVertexAttribArray(shader.vertex_loc);
	}

	mat4 rot = mat4_rotate_Z(mat4_identity(), M_PI * 0.5);
      
	for(;v1.x < v2.x; v1.x += 1){
	  vec4 v = vec4_new(v1.x * grid_width, py * grid_width,0,1);
	  v = mat4_mul_vec4(editor_tform, v);
	  mat4 cam = mat4_mul(mat4_translate(v.x, v.y + 1, v.z), rot);
	  auto loaded = simple_grid_load_polygon(gd, poly);
	  glBindBuffer(GL_ARRAY_BUFFER, loaded.gl_ref);
	  simple_grid_shader shader = simple_grid_shader_get();
	  glUseProgram(shader.shader);
	
	  glUniformMatrix4fv(shader.camera_loc,1,false, &(cam.data[0][0]));
	  glUniform4f(shader.color_loc, 0, 0, 0, 0.2);
	  glEnableVertexAttribArray(shader.vertex_loc);
	  glVertexAttribPointer(shader.vertex_loc, 2, GL_FLOAT, false, 0, 0);
	  glDrawArrays(GL_LINE_LOOP, 0, loaded.count);
	  glDisableVertexAttribArray(shader.vertex_loc);
	}
      }
    }
  }

  
  {
    u64 index = 0;
    control_pair * child_control = NULL;
    while((child_control = get_control_pair_parent(id, &index))){
      if(child_control == NULL)
	break;
      render_sub(child_control->child_id);
    }
  }
  
  u32 error = glGetError();
  if(error > 0){
    logd("GL ERROR: %i\n");
  }
}

CREATE_TABLE_DECL2(simple_graphics_control, u64, u64);
CREATE_TABLE_NP(simple_graphics_control, u64, u64);

CREATE_TABLE2(entity_target, u32, vec3);
CREATE_TABLE_DECL2(simple_game_data, u64, game_data);
CREATE_TABLE2(simple_game_data, u64, game_data);

CREATE_TABLE_DECL2(alternative_control, u64, u64);
CREATE_TABLE2(alternative_control, u64, u64);


static void command_entered(u64 id, char * command){
  u64 control = get_simple_graphics_control(id);
  editor_context editor = get_simple_graphics_editor_context(control);
  graphics_context ctx = get_graphics_context(control);
  u64 win = get_game_window(control);

  char first_part[100];
  bool first(const char * check){
    return strcmp(first_part, check) == 0;
  }

  char snd_part[100];
  bool snd(const char * check){
    return strcmp(snd_part, check) == 0;
  }
  char namebuf[100] = {0};
  
  void print_polygon(u32 index, bool print_vertexes){
    polygon_data * pd = index_table_lookup(ctx.polygon, index);
    if(pd == NULL) return;
    namebuf[0] = 0;get_desc_text2(SELECTED_POLYGON, pd->material, namebuf, array_count(namebuf));
    logd("Polygon: %i  Height:%f   verts: %i", index, pd->physical_height, pd->vertexes.count);
    vec4 color;
    if(polygon_color_try_get(ctx.poly_color, pd->material, &color))
      vec4_print(color);
    if(namebuf[0])
      logd("  '%s'", namebuf);
    logd("\n");
    if(print_vertexes){
      for(u32 i = 0; i < pd->vertexes.count; i++){
	vertex_data * vd = index_table_lookup(ctx.vertex, pd->vertexes.index + i);
	logd("Vertex %i: ", pd->vertexes.index + i);vec2_print(vd->position);logd("\n");
      }
    }
  }

  void print_entity(u32 entity_id, bool print_polygons, bool print_vertexes){
    namebuf[0] = 0;get_desc_text2(SELECTED_ENTITY, entity_id, namebuf, array_count(namebuf));
    entity_data * entity = index_table_lookup(ctx.entities, entity_id);
    logd("E: '%s' \t%i ", namebuf, entity_id); 
    u32 model_id = entity->model;
    if(model_id != 0){
      namebuf[0] = 0;get_desc_text2(SELECTED_MODEL, model_id, namebuf, array_count(namebuf));
      logd("m: %i '%s' ", model_id, namebuf);
      vec3_print(entity->position); logd("\n"); 
      if(print_polygons){
	model_data * md = index_table_lookup(ctx.models, model_id);
	for(u32 j = 0; j < md->polygons.count; j++){
	  print_polygon(j + md->polygons.index, print_vertexes);
	}
      }
    }else{
     vec3_print(entity->position); logd("\n"); 
    }
  }
  
  if(copy_nth(command, 0, first_part, array_count(first_part))){
    if(first("exit")){
	set_should_exit(control, true);
    }else if(first("list")){
      if(copy_nth(command, 1, snd_part, array_count(snd_part))){
	if(snd("polygon")){
	  char idbuf[100];
	  u64 index = 0;
	  if(copy_nth(command, 2, idbuf, array_count(idbuf))){
	    sscanf(idbuf, "%i", &index);
	    if(index > 0 && index_table_contains(ctx.polygon, (u32)index)){
	      print_polygon(index, true);
	      return;
	    }
	  }else{
	    u64 cnt = 0;
	    index_table_all(ctx.polygon, &cnt);
	    for(u64 i = 0; i < cnt; i++){
	      print_polygon(i + 1, false);
	    }
	    return;
	  }
	}
	if(snd("entity")){
	  char idbuf[100];
	  u64 index = 0;
	  if(copy_nth(command, 2, idbuf, array_count(idbuf))){
	    sscanf(idbuf, "%i", &index);
	    if(index > 0 && index_table_contains(ctx.entities, (u32)index)){
	      print_entity(index, true, false);
	      return;
	    }
	  }else{
	    u64 cnt = 0;
	    index_table_all(ctx.entities, &cnt);
	    for(u64 i = 0; i < cnt; i++){
	      print_entity(i + 1, false, false);
	    }
	    return;
	  }

	}
	
      }
	 
      logd("Listing: \n");

      u64 cnt = 0;
      index_table_all(ctx.entities, &cnt);
      logd("Entities: %i items.\n", cnt);
      bool print_vertexes = copy_nth(command, 1, snd_part, array_count(snd_part)) && snd("vertex");
      for(u64 i = 0; i < cnt; i++){
	u32 entity_id = i + 1;
	print_entity(entity_id, print_vertexes, print_vertexes);
      }
      logd("%i \n", ((u32 *)ctx.vertex->free_indexes->ptr)[0]);
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
	u32 pid = entity->polygons.index + entity->polygons.count - 1;
	polygon_data * pd = index_table_lookup(ctx.polygon, pid);
	pd->material = get_unique_number();
	polygon_color_set(ctx.poly_color, pd->material, vec4_new(0, 0, 0, 1));
	editor.selection_kind = SELECTED_POLYGON;
	editor.selected_index = entity->polygons.index + entity->polygons.count - 1;
	logd("New polygon: %i\n", editor.selected_index);
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
	graphics_context_reload_polygon(ctx, editor.selected_index);
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

	  polygon_data * pd = index_table_lookup(ctx.polygon, editor.selected_index);
	  if(pd->material == 0)
	    pd->material = get_unique_number();
	  logd("Set color: %i", pd->material);vec4_print(p);logd("\n");	  
	  polygon_color_set(ctx.poly_color, pd->material, p);
	}else if(snd("color") && editor.selection_kind == SELECTED_VERTEX) {
	  u32 polygon = vertex_get_polygon(ctx, editor.selected_index);
	  if(polygon == 0)
	    return;
	  vec4 p = vec4_zero;
	  char buf[20];
	  for(u32 i = 0 ; i < array_count(p.data); i++){
	    if(false == copy_nth(command, 2 + i, buf, array_count(buf)))
	      goto skip;
	    sscanf(buf,"%f", p.data + i);
	  }

	  polygon_data * pd = index_table_lookup(ctx.polygon, polygon);
	  if(pd->material == 0)
	    pd->material = get_unique_number();
	  logd("Set color: %i", pd->material);vec4_print(p);logd("\n");	  
	  polygon_color_set(ctx.poly_color, pd->material, p);
	  
	}else if(snd("position")&& editor.selected_index != 0  && editor.selection_kind == SELECTED_ENTITY ){
	  vec3 p = vec3_zero;
	  char buf[20];
	  for(u32 i = 0 ; i < array_count(p.data); i++){
	    if(false == copy_nth(command, 2 + i, buf, array_count(buf)))
	      break;
	    sscanf(buf,"%f", p.data + i);
	  }

	  entity_data * ed = index_table_lookup(ctx.entities, editor.selected_index);
	  if(ed != NULL){
	    ed->position = p;
	  }
	}
	if(snd("flat") && editor.selected_index != 0  && editor.selection_kind == SELECTED_POLYGON){

	  char buf[10];
	  if(copy_nth(command, 2, buf, array_count(buf))){
	    float state = 0;
	    sscanf(buf, "%f", &state);

	    polygon_data * pd = index_table_lookup(ctx.polygon, editor.selected_index);
	    pd->physical_height = state;
	    logd("FLAT: %f\n", pd->physical_height);
	  }
	}
	if(snd("name") && editor.selected_index != 0){
	  char buf[100];
	  if(copy_nth(command, 2, buf, array_count(buf))){
	    if(editor.selection_kind == SELECTED_POLYGON){
	      polygon_data * pd = index_table_lookup(ctx.polygon, editor.selected_index);
	      if(pd != NULL)
		set_desc_text2(editor.selection_kind, pd->material, buf);
	    }else{
	      set_desc_text2(editor.selection_kind, editor.selected_index, buf);
	    }
	  }
	}
	if(snd("offset") && editor.selected_index != 0  && editor.selection_kind == SELECTED_POLYGON){
	  vec2 offset = {0};
	  for(u32 i = 0;i < array_count(offset.data); i++){
	    char buf[10] = {0};
	    if(!copy_nth(command, 2 + i, buf, array_count(buf)))
	      return;
	    sscanf(buf, "%f", offset.data + i);
	  }
	  polygon_data * pd = index_table_lookup(ctx.polygon, editor.selected_index);
	  vertex_data * vd = index_table_lookup_sequence(ctx.vertex, pd->vertexes);
	  for(u32 i = 0; i < pd->vertexes.count; i++){
	    vd[i].position = vec2_add(vd[i].position, offset);
	  }
	  graphics_context_reload_polygon(ctx, editor.selected_index);
	}
	

	if(snd("zoom")){

	  char buf[20];
	  if(copy_nth(command, 2, buf, array_count(buf))){
	    float v = 1.0;
	    sscanf(buf,"%f", &v);
	    editor.zoom = v;
	  }
	  logd("ZOOM: %f\n", editor.zoom);
	  set_simple_graphics_editor_context(control, editor);
	}

	if(snd("gravity") && editor.selected_index != 0  && editor.selection_kind == SELECTED_ENTITY){
	  char buf[20];
	  if(copy_nth(command, 2, buf, array_count(buf))){
	    int v = 1;
	    sscanf(buf,"%i", &v);
	    if(v)
	      set_gravity_affects(editor.selected_index, v);
	    else
	      unset_gravity_affects(editor.selected_index);
	    logd("GRAVITY: %i\n", editor.selected_index);
	  }
	}

	if(snd("model") && editor.selection_kind == SELECTED_ENTITY && editor.selected_index != 0){
	  
	  char buf[20];
	  if(copy_nth(command, 2, buf, array_count(buf))){
	    entity_data * ed = index_table_lookup(ctx.entities, editor.selected_index);
	    u32 v = 0;
	    sscanf(buf,"%i", &v);
	    if(index_table_contains(ctx.models, v))
	      ed->model = v;
	  }
	}
	if(snd("entity")){
	  char trd_part[100];
	  if(!copy_nth(command, 2, trd_part, array_count(trd_part))) return;

	  bool trd(const char * check){
	    return strcmp(trd_part, check) == 0;
	  }
	  if(trd("type") && editor.selection_kind == SELECTED_ENTITY && editor.selected_index != 0){
	    char buf[20];
	    if(copy_nth(command, 3, buf, array_count(buf))){
	      u64 v = 0;
	      sscanf(buf,"%i", &v);
	      if(v != 0){
		logd("setting %i %i\n", editor.selected_index, v);
		set_entity_type(editor.selected_index, v);
	      }
	    }
	  }
	}

	if(snd("background")){
	  vec3 color = {0};
	  for(u32 i = 0; i < array_count(color.data); i++){
	    char buf[10] = {0};
	    if(!copy_nth(command, 2 + i, buf, array_count(buf)))
	      break;
	    sscanf(buf, "%f", &(color.data[i]));
	  }
	  set_color(win, color);
	}

	if(snd("grid-size")){
	  float v = 1;
	  char buf[10] = {0};
	  if(copy_nth(command, 2, buf, sizeof(buf))){
	    sscanf(buf, "%f", &v);
	    set_simple_graphics_editor_grid_width(control, v);
	  }
	}

	if(snd("ghost") && editor.selection_kind == SELECTED_POLYGON && editor.selected_index != 0){

	  
	  int v = 0;
	  char buf[10] = {0};
	  if(copy_nth(command, 2, buf, sizeof(buf))){
	    sscanf(buf, "%i", &v);
	  }
	  polygon_data * pd = index_table_lookup(ctx.polygon, editor.selected_index);

	  if(v){
	    logd("Setting is ghost.. for %i material: %i\n", editor.selected_index, pd->material);
	    ghost_material_set(ctx.ghost_table, pd->material, true);
	  }else{
	    logd("Unsetting is ghost.. for %i material: %i\n", editor.selected_index, pd->material);
	    ghost_material_unset(ctx.ghost_table, pd->material);
	  }
	}
	if(snd("speed") && editor.selection_kind == SELECTED_ENTITY && editor.selected_index != 0){
	  
	  

	}
	
	
    }
    else if(first("select") || first("focus")){
      bool is_focus = first("focus");
      char id_buffer[100] = {0};
      if(copy_nth(command, 1, snd_part, array_count(snd_part))
	 && copy_nth(command, 2, id_buffer, array_count(id_buffer))){
	u32 i1 = 0;
	sscanf(id_buffer, "%i", &i1);
	index_table * table = NULL;
	selected_kind kind;
	if(snd("entity")){
	  table = ctx.entities;
	  kind = SELECTED_ENTITY;
	}else if(snd("model")){
	  table = ctx.models;
	  kind = SELECTED_MODEL;
	  
	}else if(snd("polygon")){
	  table = ctx.polygon;
	  kind = SELECTED_POLYGON;
	  
	}else if(snd("vertex")){
	  table = ctx.vertex;
	  kind = SELECTED_VERTEX;
	}
	if(table != NULL && index_table_contains(table, i1)){

	  if(is_focus){
	    editor.focused_item_kind = kind;
	    editor.focused_item = i1;
	    editor.offset = vec2_zero;
	  }
	  editor.selection_kind = kind;
	  editor.selected_index = i1;
	}else{
	  editor.focused_item_kind = SELECTED_NONE;
	  editor.focused_item = 0;
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
	logd("Remove: %s, %i\n", snd_part, i1);
	if(snd("vertex")){
	  
	  u32 polygon = vertex_get_polygon(ctx, i1);
	  if(polygon == 0)
	    return;
	  
	  polygon_data * pd = index_table_lookup(ctx.polygon, polygon);
	  vertex_data * vd = index_table_lookup_sequence(ctx.vertex, pd->vertexes);
	  u32 offset = i1 - pd->vertexes.index;
	  for(u32 i = offset; i < pd->vertexes.count - 1; i++)
	    vd[i] = vd[i + 1];
	  
	  index_table_resize_sequence(ctx.vertex, &(pd->vertexes), pd->vertexes.count - 1);
	  if(pd != NULL && pd->material != 0){
	    u32 idx = index_table_alloc(ctx.polygons_to_delete);
	    u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
	    ptr[0] = pd->material;
	  }
	}
	else if(snd("polygon")){
	  char buf[100] = {0};
	  u32 startidx = 0;
	  u32 endidx = 0;
	  if(copy_nth(command, 2, buf, sizeof(buf)))
	    sscanf(buf, "%i", &startidx);
	  if(copy_nth(command, 3, buf, sizeof(buf)))
	    sscanf(buf, "%i", &endidx);
	  if(startidx != 0 && endidx >= startidx){
	    index_table_sequence seq = {.index = startidx, .count = endidx - startidx + 1};
	    index_table_resize_sequence(ctx.vertex, &(seq), 0);
	  }else{
	    polygon_data * pd = index_table_lookup(ctx.polygon, i1);
	    index_table_resize_sequence(ctx.vertex, &(pd->vertexes), 0);
	  }
	}
      }
      set_simple_graphics_editor_context(control, editor);
    }else if(first("clear")){
      char id_buffer[100] = {0};
      if(copy_nth(command, 1, snd_part, array_count(snd_part))){
	u32 i1 = 0;
	if(copy_nth(command, 2, id_buffer, array_count(id_buffer))){
	  sscanf(id_buffer, "%i", &i1);
	}else{
	  i1 = editor.selected_index;
	}
	logd("Clear: %s, %i\n", snd_part, i1);
	if(snd("polygon")){
	  polygon_data * pd = index_table_lookup(ctx.polygon, i1);
	  index_table_resize_sequence(ctx.vertex, &(pd->vertexes), 0);
	  u32 idx = index_table_alloc(ctx.polygons_to_delete);
	  u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
	  ptr[0] = pd->material;
	}
      }
    }else if(first("recenter")){
      // recenter the model based on average vertex positions of a polygon.
      u32 model = 0;
      u32 polygon = 0;
      if(editor.selection_kind == SELECTED_MODEL)
	model = editor.selected_index;
      
      char buf[100];
      if(copy_nth(command,1, buf, array_count(buf))){
	sscanf(buf, "%i", &model);
      }

      if(model != 0){
	model_data * md = index_table_lookup(ctx.models, model);
	polygon = md->polygons.index;
      }

      if(copy_nth(command,2, buf, array_count(buf))){
	sscanf(buf, "%i", &polygon);
      }
      logd("Recenter: %i %i\n", model, polygon);
      if(model == 0){
	loge("ERROR: model == 0");
	return;
      }

      if(polygon == 0){
	loge("ERROR: polygon == 0");
	return;
      }
      vec2 avg = vec2_infinity;
      {
	polygon_data * pd = index_table_lookup(ctx.polygon, polygon);
	vertex_data * v = index_table_lookup_sequence(ctx.vertex, pd->vertexes);
	for(u32 i = 0; i < pd->vertexes.count; i++){
	  avg = vec2_min(avg, v[i].position);
	}
	//avg = vec2_scale(avg, 1.0f / pd->vertexes.count);
      }
      model_data * md = index_table_lookup(ctx.models, model);
      polygon_data * pd = index_table_lookup_sequence(ctx.polygon, md->polygons);
      vec2_print(avg);logd("\n");
      if(avg.x == f32_infinity || avg.y == f32_infinity){
	ASSERT(false);
	return;
      }
      
      for(u32 i = 0; i < md->polygons.count; i++){
	index_table_sequence seq = pd[i].vertexes;
	vertex_data * v = index_table_lookup_sequence(ctx.vertex, seq);
	for(u32 j = 0; j < seq.count; j++)
	  v[j].position = vec2_sub(v[j].position, avg);
	graphics_context_reload_polygon(ctx, md->polygons.index + i);
      }
    }else if(first("scale")){
      if(copy_nth(command, 1, snd_part, array_count(snd_part))){
	if(snd("model")){
	  u32 model = 0;
	  f32 scale = 1.0;
	  char arg[20];
	  if(copy_nth(command, 3, arg, sizeof(arg))){
	    sscanf(arg, "%f", &scale);
	    copy_nth(command, 2, arg, sizeof(arg));
	    sscanf(arg, "%i", &model);
	  }else{
	    copy_nth(command, 2, arg, sizeof(arg));
	    sscanf(arg, "%f", &scale);
	  }
	  if(model == 0)
	    return;
	  model_data * md = index_table_lookup(ctx.models, model);
	  polygon_data * pd = index_table_lookup_sequence(ctx.polygon, md->polygons);
	  for(u32 i = 0; i < md->polygons.count; i++){
	    index_table_sequence seq = pd[i].vertexes;
	    vertex_data * v = index_table_lookup_sequence(ctx.vertex, seq);
	    for(u32 j = 0; j < seq.count; j++)
	      v[j].position = vec2_scale(v[j].position, scale);
	    graphics_context_reload_polygon(ctx, md->polygons.index + i);
	  }
	} 
      }
    }
    else if(first("moveup") && editor.selection_kind == SELECTED_POLYGON){
      u32 model = polygon_get_model(ctx, editor.selected_index);
      if(model != 0){
	model_data * md = index_table_lookup(ctx.models, model);
	if(editor.selected_index > md->polygons.index
	   && editor.selected_index < md->polygons.index + md->polygons.count){
	  polygon_data * pd = index_table_lookup_sequence(ctx.polygon, md->polygons);
	  u32 a = editor.selected_index - md->polygons.index;
	  u32 b = a - 1;
	  if(a > 0)
	    SWAP(pd[a], pd[b]);
	  graphics_context_reload_polygon(ctx, md->polygons.index + a);
	  graphics_context_reload_polygon(ctx, md->polygons.index + b);
	}
      }
    }else if(first("optimize")){
      {
	u64 cnt = 0;
	index_table_all(ctx.entities, &cnt);
	for(u64 i = 0; i < cnt; i++){
	  u32 entity_id = i + 1;
	  entity_data * entity = index_table_lookup(ctx.entities, entity_id);
	  u32 model_id = entity->model;
	  if(model_id != 0){
	    model_data * md = index_table_lookup(ctx.models, model_id);
	    for(u32 j = 0; j < md->polygons.count; j++){
	      polygon_data * pd = index_table_lookup(ctx.polygon, j + md->polygons.index);
	      index_table_resize_sequence(ctx.vertex, &(pd->vertexes), pd->vertexes.count);
	    }
	  }
	}
      }
    
      index_table_optimize(ctx.polygon);
      index_table_optimize(ctx.vertex);
      index_table_optimize(ctx.entities);
      index_table_optimize(ctx.models);
      logd("Optimized!\n");
    }else if(first("backup")){
      {
	iron_process cdp = {};
	const char * args[] = {"data", 0};
	ASSERT(0 == iron_process_run("cd", (const char **)args, &cdp));
	iron_process_wait(cdp, 1000000);
      }
      {
	iron_process cdp = {};
	const char * args[] = {"commit", "-a", "-m" ,"\"backup\"", 0};
	ASSERT(0 == iron_process_run("/usr/bin/git", (const char **)args, &cdp));
	iron_process_wait(cdp, 1000000);
      }
      {
	iron_process cdp = {};
	const char * args[] = {"..", 0};
	ASSERT(0 == iron_process_run("cd", (const char **)args, &cdp));
	iron_process_wait(cdp, 1000000);
      }
    }else if(first("snap-to-grid")){
      editor.snap_to_grid = !editor.snap_to_grid;
      set_simple_graphics_editor_context(control, editor);
    }else if(first("load_module")){
      char buf[41];
      if(copy_nth(command, 1, buf, sizeof(buf))){
	u64 cnt = 0;
	module_name * modules = index_table_all(ctx.loaded_modules, &cnt);
	for(u64 i = 0; i < cnt; i++){
	  if(strcmp(modules[i].name, buf) == 0){
	    return;
	  }
	}
	load_module(&ctx, buf);
      }
    }

    else{
      simple_graphics_editor_fcn fcns[10];
      u32 fcn_id[10];
      u64 idx = 0;
      u32 count = 0;
      while((count = simple_game_editor_fcn_iter_all(ctx.editor_functions, fcn_id, fcns, array_count(fcns), &idx)))
	{
	for(u32 i = 0; i < count; i++){
	  if(fcns[i](&ctx, &editor, command)){
	    set_simple_graphics_editor_context(control, editor);
	    return;
	  }
	}
      }

      
      logd("Unkown command!\n");
    }
  }
  //logd("COMMAND ENTERED %i %s\n", id, command);
}

CREATE_TABLE_DECL2(console_history_index, u64, u32);
CREATE_TABLE2(console_history_index, u64, u32);


static void console_handle_key(u64 console, int key, int mods, int action){

  if(action == key_release)
    return;
  if(key == key_tab && action == key_press){
    u64 control = get_simple_graphics_control(console);
    u64 parent_id = control_pair_get_parent(control);
    control_pair * p = add_control(parent_id, control);
    p->child_id = get_alternative_control(control);
    set_focused_element(parent_id, p->child_id);
    return;
  }else if(key == key_backspace && mods == mod_ctrl){
    logd("MODS: %i\n", mods);
    u64 control = get_simple_graphics_control(console);
    graphics_context ctx = get_graphics_context(control);
    editor_context editor = get_simple_graphics_editor_context(control);
    if(editor.selection_kind == SELECTED_POLYGON){
      polygon_data * pd = index_table_lookup(ctx.polygon, editor.selected_index);
      if(pd == NULL)
	return;
      u32 pcnt = pd->vertexes.count;
      if(pcnt > 0)
	index_table_resize_sequence(ctx.vertex, &(pd->vertexes), pcnt - 1);
      logd("COUNT: %i\n", pcnt);
      loaded_polygon_data loaded;
      if(loaded_polygon_try_get(ctx.gpu_poly, pd->material, &loaded)){
	u32 idx = index_table_alloc(ctx.polygons_to_delete);
	u32 * ptr = index_table_lookup(ctx.polygons_to_delete, idx);
	ptr[0] = pd->material;
      }
    }
    return;
  }
  
  //logd("%i %i %i\n", console, key, mods);
  u64 histcnt = get_console_history_cnt(console);
  u64 history[histcnt + 1];
  u64 history_cnt = get_console_history(console, history, array_count(history));
  
  u32 idx = get_console_history_index(console);
  if(key == key_up){
    idx += 1;
  }else if(key == key_down){
    if(idx != 0)
      idx -= 1;
  }else{
    unset_console_history_index(console);
    goto skip;
  }

  if(idx > history_cnt){
    idx = 0;
  }
  if(idx == 0)
    unset_console_history_index(console);
  else
    set_console_history_index(console, idx);
  if(idx > 0){
    char buffer[200] = {0};
    if(console_get_history(console, idx - 1, buffer, array_count(buffer))){
      //logd("SETTING TEXT TO '%s'\n", buffer);
      remove_text(console);
      set_text(console, buffer);
    }
  }
  //logd("HIST IDX: %i\n", idx);
  
 skip:;
  CALL_BASE_METHOD(console, key_handler_method, key, mods, action);
}


void simple_grid_measure(u64 id, vec2 * size){
  UNUSED(id);
  *size = shared_size;
  graphics_context gd = get_graphics_context(id);
  gd.render_size = *size;
  set_graphics_context(id, gd);  
}



static void game_handle_key(u64 console, int key, int mods, int action){
  if(key == key_tab && action == key_press){

    u64 parent_id = control_pair_get_parent(console);
    control_pair * p = gui_get_control(parent_id, console);
    p->child_id = get_alternative_control(console);
    u64 keybuf[10];
    u64 valuebuf[10];
    u64 idx = 0;
    u64 cnt = 0;
    while(0 != (cnt = iter_all_simple_graphics_control(keybuf, valuebuf, array_count(keybuf), &idx))){
      for(u32 i = 0; i < cnt; i++){
	if(valuebuf[i] == p->child_id){
	  set_focused_element(parent_id, keybuf[i]);
	  return;
	}
      }
    }
  }
  if(key == key_space && action == key_press){
    u64 ctl = get_alternative_control(console);
    graphics_context ctx = get_graphics_context(ctl);
    game_data gd = get_simple_game_data(console);
    if(gd.selected_entity != 0){
      vec3 current_impulse;
      if(!try_get_current_impulse(gd.selected_entity, &current_impulse))
	set_current_impulse(gd.selected_entity, vec3_new(0,0.05, 0));
    }
    UNUSED(ctx);
  }
  UNUSED(mods);
}
struct detect_collision_aabb{
    vec3 min, max;
  };

void simple_game_build_aabb_table(u32 * entities, u32 entitycnt, graphics_context gd, index_table * aabb_table){
  if(entitycnt == 0)
    return;
  index_table_clear(aabb_table);
  index_table_sequence aabbseq = index_table_alloc_sequence(aabb_table, entitycnt);
  struct detect_collision_aabb * aabbs = index_table_lookup_sequence(aabb_table, aabbseq);
  memset(aabbs, 0, aabbseq.count * sizeof(struct detect_collision_aabb));

  for(u64 i = 0; i < entitycnt; i++){
    u64 entity = entities[i];
    if(entity == 0)
      continue;
    entity_data * ed = index_table_lookup(gd.entities, entity);
    if(ed == NULL)
      continue;
    if(ed->model == 0)
      continue;
    model_data * md = index_table_lookup(gd.models, ed->model);
    if(md == NULL) continue;
    polygon_data * pd = index_table_lookup_sequence(gd.polygon, md->polygons);
    if(pd == NULL) continue;
    vec3 min = vec3_new1(f32_infinity), max = vec3_new1(f32_negative_infinity);
    u64 pdcnt = md->polygons.count;
    bool any = false;
    for(u32 i = 0; i < pdcnt; i++){
      
      bool flat = pd[i].physical_height != 0.0f;
      u64 vcnt1 = pd[i].vertexes.count;
      vertex_data * vd1 = index_table_lookup_sequence(gd.vertex, pd[i].vertexes);
      for(u32 j = 0; j < vcnt1; j++){
	vec2 pos = vd1[j].position;
	if(flat){
	  any = true;
	  if(pd[i].physical_height < 0){
	    min = vec3_min(min, vec3_new(pos.x, pd[i].physical_height, pos.y));
	    max = vec3_max(max, vec3_new(pos.x, 0, pos.y));
	  }else{
	    min = vec3_min(min, vec3_new(pos.x, 0, pos.y));
	    max = vec3_max(max, vec3_new(pos.x, pd[i].physical_height, pos.y));
	  }
	}
      }
    }
    if(any){
      aabbs[i].min = vec3_add(ed->position, min);
      aabbs[i].max = vec3_add(ed->position, max);
    }else{
      aabbs[i].min = min;
      aabbs[i].max = max;
    }
  }
}

bool detect_collision(graphics_context gd, u32 entity1, struct detect_collision_aabb aabb1, u32 entity2, struct detect_collision_aabb aabb2, index_table * result_table){
  if(aabb2.min.x == f32_infinity)
    return false;
  if(aabb2.min.x == aabb2.max.x)
    return false;
  if(aabb1.min.x == f32_infinity)
    return false;
  if(aabb1.max.x == aabb1.min.x)
    return false;
  if(aabb1.min.x > aabb2.max.x || aabb1.max.x < aabb2.min.x
     ||aabb1.min.z > aabb2.max.z || aabb1.max.z < aabb2.min.z
     ||aabb1.min.y > aabb2.max.y || aabb1.max.y < aabb2.min.y)
    return false;
  
  bool grav1 = get_gravity_affects(entity1);
  bool grav2 = get_gravity_affects(entity2);
  if(!(grav1 || grav2))
    return false;
  entity_data * ed2 = index_table_lookup(gd.entities, entity2);
  if(ed2 == NULL)
    return false;
  entity_data * ed = index_table_lookup(gd.entities, entity1);
  if(ed == NULL)
    return false;
  if(ed->model == 0) return false;      
  model_data * md = index_table_lookup(gd.models, ed->model);
  u64 pdcnt = md->polygons.count;
  polygon_data * pd = index_table_lookup_sequence(gd.polygon, md->polygons);
  vec3 position = ed->position;
  
  vec3 position2 = ed2->position;
  if(ed2->model == 0) return false;
  model_data * md2 = index_table_lookup(gd.models, ed2->model);
  u64 pd2cnt = md2->polygons.count;
  polygon_data * pd2 = index_table_lookup_sequence(gd.polygon, md2->polygons);
  
  if(pd2 == NULL) return false;
  bool any_collided = false;
  for(u32 i = 0; i < pdcnt; i++){
    if(pd[i].physical_height == 0.0f) continue;
    for(u32 j = 0; j < pd2cnt; j++){
      if(pd2[j].physical_height == 0.0f) continue;
      u64 vcnt1 = pd[i].vertexes.count;
      u64 vcnt2 = pd2[j].vertexes.count;

      vertex_data * vd1 = index_table_lookup_sequence(gd.vertex, pd[i].vertexes);
      vertex_data * vd2 = index_table_lookup_sequence(gd.vertex, pd2[j].vertexes);

      if(vd1 == NULL || vd2 == NULL) continue;
      bool cw1 = true;
      vec3 offset = vec3_sub(position, position2);
      {
	// detect collisions along the y axis.
	float y1 = 0, y2 = pd2[j].physical_height,
	  y3 = offset.y, y4 = offset.y + pd[i].physical_height;
	if(y2 < y1) SWAP(y2, y1);
	if(y4 < y3) SWAP(y4, y3);
	if(y1 > y4 || y2 < y3)
	  continue;
      }

      for(u64 k1 = 2; k1 < vcnt1; k1++){
	vec2 verts1[3];
	if(cw1){
	  verts1[0] = vd1[k1 - 2].position;
	  verts1[1] = vd1[k1 - 1].position;
	  verts1[2] = vd1[k1].position;
	  cw1 = false;
	}else{
	  verts1[0] = vd1[k1].position;
	  verts1[1] = vd1[k1 - 1].position;
	  verts1[2] = vd1[k1 - 2].position;
	  cw1 = true;
	}
		
	for(u32 kk = 0; kk < 3; kk++){
	  vec2 off = {.x = offset.x, .y = offset.z};
	  verts1[kk] = vec2_add(verts1[kk], off);
	}
		
	bool cw2 = true;
	for(u64 k2 = 2; k2 < vcnt2; k2++){
	  vec2 verts2[3];
	  if(cw2){
	    verts2[0] = vd2[k2 - 2].position;
	    verts2[1] = vd2[k2 - 1].position;
	    verts2[2] = vd2[k2].position;
	    cw2 = false;
	  }else{
	    verts2[0] = vd2[k2].position;
	    verts2[1] = vd2[k2 - 1].position;
	    verts2[2] = vd2[k2 - 2].position;
	    cw2 = true;
	  }
	  // verts1 and verts2 are two triangles.
	  int cols = 0;
	  for(u32 ii = 0; ii < 3; ii++){
	    u32 ii2 = ii == 2 ? 0 : ii + 1;
	    u32 ii3 = ii == 0 ? 2 : ii - 1;
	    vec2 a = verts1[ii];
	    vec2 b = verts1[ii2];
	    vec2 d = vec2_normalize(vec2_sub(b, a));
	    d = vec2_new(d.y, -d.x);
	    vec2 e = verts1[ii3];
	    float xe = vec2_mul_inner(d, vec2_sub(e, a));
	    bool gotcol = false;
	    for(u32 jj = 0; jj < 3; jj++){
	      u32 jj2 = jj == 2 ? 0 : jj + 1;
	      vec2 a2 = vec2_sub(verts2[jj], a);
	      vec2 b2 = vec2_sub(verts2[jj2], a);
	      float x = vec2_mul_inner(d, a2);
	      float y = vec2_mul_inner(d, b2);
	      if(x > y)
		SWAP(x, y);
	      if(x < xe && y > 0)
		gotcol = true;
	    }
	    if(gotcol)
	      cols += 1;
	  }
	      
	  for(u32 ii = 0; ii < 3; ii++){
	    u32 ii2 = ii == 2 ? 0 : ii + 1;
	    u32 ii3 = ii == 0 ? 2 : ii - 1;
	    vec2 a = verts2[ii];
	    vec2 b = verts2[ii2];
	    vec2 d = vec2_normalize(vec2_sub(b, a));
	    d = vec2_new(d.y, -d.x);
	    vec2 e = verts2[ii3];
	    float xe = vec2_mul_inner(d, vec2_sub(e, a));
	    bool gotcol = false;
	    for(u32 jj = 0; jj < 3; jj++){
	      u32 jj2 = jj == 2 ? 0 : jj + 1;
	      vec2 a2 = vec2_sub(verts1[jj], a);
	      vec2 b2 = vec2_sub(verts1[jj2], a);
	      float x = vec2_mul_inner(d, a2);
	      float y = vec2_mul_inner(d, b2);
	      if(x > y)
		SWAP(x, y);
	      if(x < xe && y > 0){
		gotcol = true;
	      }
	    }
	    if(gotcol)
	      cols += 1;
	  }
	  if(cols == 6){ // Found collision on all 6 axis
	    collision_data cd;
	    cd.entity1.entity = entity1;
	    cd.entity1.model = ed->model;
	    cd.entity1.polygon_offset = md->polygons.index + i;
	    cd.entity1.vertex_offset = k1;
	    cd.entity1.material = pd[i].material;
	    cd.entity2.entity = entity2;
	    cd.entity2.model = ed2->model;
	    cd.entity2.polygon_offset = md2->polygons.index + i;
	    cd.entity2.vertex_offset = k2;
	    cd.entity2.material = pd2[j].material;
	    u32 idx = index_table_alloc(result_table);
	    *((collision_data *)index_table_lookup(result_table, idx)) = cd;
	    any_collided = true;
	  }
	}
      }
    }
  }
  return any_collided;
}
void detect_collisions(u32 * entities, u32 entitycnt, graphics_context gd, index_table * result_table){
  
  static index_table * aabb_table = NULL;
  if(aabb_table == NULL)
    aabb_table = index_table_create(NULL, sizeof(struct detect_collision_aabb));
  simple_game_build_aabb_table(entities, entitycnt, gd, aabb_table);
  u64 cnt = 0;
  struct detect_collision_aabb * aabbs = index_table_all(aabb_table, &cnt);
  ASSERT(cnt == entitycnt);
  
  for(u64 i = 0; i < entitycnt; i++){
    for(u64 j = i + 1; j < entitycnt; j++){
      detect_collision(gd, entities[i], aabbs[i], entities[j], aabbs[j],result_table);
    }
  }
}

void detect_collisions_one_way(graphics_context gd, u32 * entities1, u32 entity1_cnt, u32 * entities2, u32 entity2_cnt, index_table * result_table){
  ASSERT(result_table != NULL);
  struct detect_collision_aabb * aabbs1 = NULL;
  struct detect_collision_aabb * aabbs2 = NULL;

  { // build aabb_table 1
    static index_table * aabb_table = NULL;
    if(aabb_table == NULL)
      aabb_table = index_table_create(NULL, sizeof(struct detect_collision_aabb));
    simple_game_build_aabb_table(entities1, entity1_cnt, gd, aabb_table);
    u64 cnt = 0;

    aabbs1 = index_table_all(aabb_table, &cnt);
    ASSERT(cnt == entity1_cnt);
  }

  { // build aabb_table 2
    static index_table * aabb_table = NULL;
    if(aabb_table == NULL)
      aabb_table = index_table_create(NULL, sizeof(struct detect_collision_aabb));
    simple_game_build_aabb_table(entities2, entity2_cnt, gd, aabb_table);
    u64 cnt = 0;

    aabbs2 = index_table_all(aabb_table, &cnt);
    ASSERT(cnt == entity2_cnt);
  } 

  for(u64 i = 0; i < entity1_cnt; i++){
    for(u64 j = 0; j < entity2_cnt; j++){
      detect_collision(gd, entities1[i], aabbs1[i], entities2[j], aabbs2[j],result_table);
    }
  }
}


CREATE_TABLE_DECL2(entity_index_lookup, u32, u32);
CREATE_TABLE_NP(entity_index_lookup, u32, u32);
void simple_grid_render(u64 id){
  game_data _gd = get_simple_game_data(id);
  vec2 offset = _gd.offset;
  graphics_context gd = get_graphics_context(get_alternative_control(id));
  gd.game_data = &_gd;
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
  u64 idx = 0;

  u64 count = active_entities_count(gd.active_entities);
  u32 entities[count];
  u32 indexes[count];

  vec3 prevp[count];
  bool moved[count];
  bool unused[count];
  idx = 0;
  static entity_index_lookup_table * entity_lookup = NULL;
  if(entity_lookup == NULL)
    entity_lookup = entity_index_lookup_table_create(NULL);
  entity_index_lookup_clear(entity_lookup);
  
  entity_2_collisions_clear(gd.collisions_2_table);

  active_entities_iter_all(gd.active_entities, entities, unused, count, &idx);
  { // check collisions due to move.
    for(u32 i = 0; i < count ;i ++){
      moved[i] = false;
      u32 entity = entities[i];
      entity_index_lookup_set(entity_lookup, entity, i);
      vec3 target;
      if(try_get_entity_target(entity, &target)){
	entity_data * ed = index_table_lookup(gd.entities, entity);
	target.y = ed->position.y;
	vec3 d = vec3_sub(target, ed->position);
	float l = vec3_len(d);
	if(l <= 0.01){
	  unset_entity_target(entity);
	}else{
	  float speed = entity_speed_get(gd.entity_speed, entity);
	  auto p2 = vec3_add(ed->position, vec3_scale(d, speed / l));
	  prevp[i] = ed->position;
	  ed->position = p2;
	  moved[i] = true;
	}
      }
    }

    {
      
      index_table_clear(gd.collision_table);
      detect_collisions(entities, count, gd, gd.collision_table);
      u64 collisions = 0;

      collision_data * cd = index_table_all(gd.collision_table, &collisions);
      for(u32 i = 0; i < collisions; i++){
	collision_data cd1 = cd[i];

	if(ghost_material_get(gd.ghost_table, cd1.entity1.material)
	   || ghost_material_get(gd.ghost_table, cd1.entity2.material))
	  continue;
	
	for(int _i = 0; _i < 2; _i++){
	  u32 idx = entity_index_lookup_get(entity_lookup, cd1.entity1.entity);
	  if(moved[idx]){
	    entity_data * ed = index_table_lookup(gd.entities, cd1.entity1.entity);
	    ed->position = prevp[idx];
	  }
	  SWAP(cd1.entity1, cd1.entity2);
	}
      }
    }
  }

  if(true){ // check collisions due to move.
    vec3 gravity = vec3_new(0, -0.01, 0);
    for(u32 i = 0; i < count ;i ++){
      moved[i] = false;
      u32 entity = entities[i];

      vec3 impulse = vec3_zero;
      bool grav = get_gravity_affects(entity);
      bool hasimpulse = try_get_current_impulse(entity, &impulse);

      if(try_get_current_impulse(entity, &impulse) || grav){
	entity_data * ed = index_table_lookup(gd.entities, entity);
	prevp[i] = ed->position;
	
	auto p2 = ed->position;
	if(hasimpulse){
	  
	  if(grav){
	    impulse = vec3_add(impulse, gravity);
	    p2 = vec3_add(p2, impulse);
	    if(vec3_len(vec3_sub(impulse, gravity)) < 0.01)
	      unset_current_impulse(entity);
	    else
	      set_current_impulse(entity, vec3_scale(impulse, 0.9));
	    
	  }else{
	    p2 = vec3_add(p2, impulse);
	    if(vec3_len(impulse) < 0.1)
	      unset_current_impulse(entity);
	    else
	      set_current_impulse(entity, vec3_scale(impulse, 0.8));
	  }
	}else{
	  if(grav)
	    p2 = vec3_add(p2, gravity);
	}

	ed->position = p2;
	moved[i] = true;
      }
    }

    {
      
      index_table_clear(gd.collision_table);
      detect_collisions(entities, count, gd, gd.collision_table);
      u64 collisions = 0;
      collision_data * cd = index_table_all(gd.collision_table, &collisions);
      u32 e1[collisions];
      u32 e2[collisions];
      for(u32 i = 0; i < collisions; i++){
	e1[i] = cd[i].entity1.entity;
	e2[i] = cd[i].entity2.entity;
      }

      //entity_2_collisions_insert(gd.collisions_2_table, e1, e2, collisions);
      //entity_2_collisions_insert(gd.collisions_2_table, e2, e1, collisions);

      int keycmp32(const u32 * k1,const  u32 * k2){
	if(*k1 > *k2)
	  return 1;
	else if(*k1 == *k2)
	  return 0;
	else return -1;
      }
      
      qsort(e1, collisions, sizeof(*e1), (void *)keycmp32);
      qsort(e2, collisions, sizeof(*e2), (void *)keycmp32);
      for(u32 i = 0; i < count; i++){
	if(moved[i] == false) continue;
	if(bsearch(entities + i, e1, collisions, sizeof(entities[i]), (void *) keycmp32)
	   ||bsearch(entities + i, e2, collisions, sizeof(entities[i]), (void *) keycmp32)){
	  entity_data * ed = index_table_lookup(gd.entities, entities[i]);
	  ed->position = prevp[i];
	}	
      }
    }
  }
  
  { // update functions
    simple_game_update_fcn fcns[30];
    u32 fcn_id[array_count(fcns)];
    u64 idx = 0;
    u64 update_fcns = 0;
    while((update_fcns = simple_game_update_iter_all(gd.game_update_functions, fcn_id, fcns, array_count(fcns), &idx))){
      for(u32 i = 0; i < update_fcns; i++){
	fcns[i](&gd);
      }
    }
    set_simple_game_data(id, *gd.game_data);
  }

  // depth is used for simple depth sorting
  // this is important to get correct alpha blending.
  float depth[count];
  idx = 0;
  mat4 cammat = mat4_identity();
  cammat.m22 = 0;
  cammat.m12 = 0;
  cammat.m02 = 0;
  cammat.m21 = 1;
  cammat = mat4_mul(cammat, mat4_scaled(1.0 / _gd.zoom, 1.0 / _gd.zoom,1.0 / _gd.zoom));
  for(u64 i = 0; i < count; i++){
    entity_data * ed = index_table_lookup(gd.entities, entities[i]);
    depth[i] = ed->position.y;
    indexes[i] = i;
  }

  int ecmp(const u32 * k1,const  u32 * k2){
    if(depth[*k1] > depth[*k2])
      return 1;
      else if(depth[*k1] == depth[*k2])
	return 0;
      else return -1;
  }
  qsort(indexes, count, sizeof(indexes[0]), (void *)ecmp);
  
  for(u64 i = 0; i < count; i++){
    u32 entity = entities[indexes[i]];
    
    entity_data * ed = index_table_lookup(gd.entities, entity);
    
    vec3 p = ed->position;
    mat4 tform = mat4_translate(p.x - offset.x, p.y - offset.y, p.z);
    tform = mat4_mul(cammat, tform);
    if(ed->model != 0){
      model_data * model = index_table_lookup(gd.models, ed->model);
      
      for(u32 j = 0; j < model->polygons.count; j++){
	u32 index = j + model->polygons.index;	
	simple_grid_render_gl(gd, index, tform, false, p.z);
      }   
    }
  }

  u32 error = glGetError();
  if(error > 0)
    logd("GL ERROR: %i\n", error);
  clear_game_event();
}

void simple_grid_game_mouse_over_func(u64 grid_id, double x, double y, u64 method){
  u64 ctl = get_alternative_control(grid_id);
  graphics_context ctx = get_graphics_context(ctl);
  
  if(method == 0){
    {
      vec2 p = graphics_context_pixel_to_screen(ctx, vec2_new(x, y));

      game_data gd = get_simple_game_data(grid_id);
      if(gd.mouse_state){
	vec2 lastp = ctx.pointer_position;
	vec2 d = vec2_sub(p, graphics_context_pixel_to_screen(ctx, lastp));
	d = vec2_scale(d, gd.zoom);
	gd.offset = vec2_sub(gd.offset, d);
      }
      
      gd.last_mouse_position = p;
      set_simple_game_data(grid_id, gd);
      ctx.pointer_position = vec2_new(x, y);
      set_graphics_context(ctl, ctx);
      return;
    }
  }
  ctx.pointer_position = vec2_new(x, y);
  set_graphics_context(ctl, ctx);
  if(method != 0){
    auto on_mouse_over = get_method(grid_id, method);
    on_mouse_over(grid_id, x, y, 0);
    return;
  }
}

void simple_game_point_collision(graphics_context ctx, u32 * entities, u32 entity_count, vec2 loc, index_table * collisiontable){
  ASSERT(collisiontable->element_size == sizeof(entity_local_data));
  vec2 p = loc;
  {
    for(u64 i = 0; i < entity_count; i++){
      if(entities[i] == 0) continue;
      entity_data * ed = index_table_lookup(ctx.entities, entities[i]);
      vec3 position = ed->position;
      UNUSED(position);
      if(ed == NULL)
	continue;
      vec2 p2 = vec2_sub(p, position.xy);
      p2.y -= position.z;
      if(ed->model == 0)
	continue;
      model_data * md = index_table_lookup(ctx.models, ed->model);
      if(md == NULL)
	continue;
      u64 pdcnt = md->polygons.count;
      polygon_data * pd = index_table_lookup_sequence(ctx.polygon, md->polygons);
      if(pd == NULL) continue;
      for(u64 j = 0; j < pdcnt; j++){
	u64 vcnt = pd[j].vertexes.count;
	vertex_data * vd = index_table_lookup_sequence(ctx.vertex, pd[j].vertexes);
	if(vd == NULL) continue;
	bool cw = true;
	for(u64 k = 2; k < vcnt; k++){
	  vec2 verts[3];
	  if(cw){
	    verts[0] = vd[k-2].position;
	    verts[1] = vd[k-1].position;
	    verts[2] = vd[k].position;
	    cw = false;
	  }else{
	    verts[0] = vd[k].position;
	    verts[1] = vd[k-1].position;
	    verts[2] = vd[k - 2].position;
	    cw = true;
	  }
	  vec2 d0 = vec2_sub(verts[1], verts[0]);
	  vec2 pn = vec2_sub(p2, verts[0]);
	  vec2 n2 = vec2_new(pn.y, -pn.x);
	  float dp0 = vec2_mul_inner(n2, d0);
	  
	  d0 = vec2_sub(verts[2], verts[1]);
	  pn = vec2_sub(p2, verts[1]);
	  n2 = vec2_new(pn.y, -pn.x);
	  float dp1 = vec2_mul_inner(n2, d0);
	  
	  d0 = vec2_sub(verts[0], verts[2]);
	  pn = vec2_sub(p2, verts[2]);
	  n2 = vec2_new(pn.y, -pn.x);
	  float dp2 = vec2_mul_inner(n2, d0);
	  if(dp0 < 0 && dp1 < 0 && dp2 < 0){
	    u32 idx = index_table_alloc(collisiontable);
	    entity_local_data * d =index_table_lookup(collisiontable, idx);
	    d->entity = entities[i];
	    d->model = ed->model;
	    d->polygon_offset = md->polygons.index + j;
	    d->vertex_offset = k;
	    d->material = pd[j].material;
	  }
	}
      }
    }
  }
}

CREATE_TABLE2(game_event, u32, game_event);
u32 game_event_index_new(){
  static u32 * ptr = NULL;
  if(ptr == NULL){
    auto area = create_mem_area("simple/game_event_number");
    mem_area_realloc(area, sizeof(u32));
    ptr = area->ptr;
  }
  return (*ptr)++;
}

void simple_grid_game_mouse_down_func(u64 grid_id, double x, double y, u64 method){
  UNUSED(method);
  u64 ctl = get_alternative_control(grid_id);
  graphics_context ctx = get_graphics_context(ctl);
  game_data gd = get_simple_game_data(grid_id);
  vec2 p = graphics_context_pixel_to_screen(ctx, vec2_new(x, y));
  p = vec2_scale(p, gd.zoom);
  vec2 v2 = vec2_add(p, gd.offset);
  if(mouse_button_action != mouse_button_repeat)
    set_game_event(game_event_index_new(), (game_event){.kind = GAME_EVENT_MOUSE_BUTTON, .mouse_button.game_position = v2, .mouse_button.button = mouse_button_button,
	  .mouse_button.pressed = mouse_button_action == mouse_button_press});
  if(mouse_button_button == mouse_button_right && mouse_button_action != mouse_button_repeat){
    if(gd.mouse_state){
      //vec2 lastp = gd.last_mouse_position;
      //vec2 d = vec2_sub(p, lastp);
      //gd.offset = vec2_add(vec2_scale(d, gd.zoom), gd.offset);
    }
    gd.last_mouse_position = p;
    
    gd.mouse_state = mouse_button_action == mouse_button_press;
    u64 win = get_game_window(grid_id);
    if(gd.mouse_state){
      gui_acquire_mouse_capture(win, grid_id);
      gui_set_cursor_mode(win, gui_window_cursor_disabled);
    }else{
      gui_release_mouse_capture(win, grid_id);
      gui_set_cursor_mode(win, gui_window_cursor_normal);
    }
    set_simple_game_data(grid_id, gd);
    return;
  }
}

void simple_grid_game_measure(u64 id, vec2 * size){
  UNUSED(id);
  *size = shared_size;
  u64 ctl = get_alternative_control(id);
  graphics_context gd = get_graphics_context(ctl);
  gd.render_size = *size;
  set_graphics_context(ctl, gd);  
}

#include "console.h"
void simple_graphics_editor_load(u64 id, u64 win_id){
  define_method(id, render_control_method, (method)simple_graphics_editor_render);
  define_method(id, measure_control_method, (method)simple_grid_measure);
  define_method(id, mouse_over_method, (method)simple_grid_mouse_over_func);
  define_method(id, mouse_down_method, (method) simple_grid_mouse_down_func);

  graphics_context gd;
  graphics_context_load(&gd);
  
  set_graphics_context(id, gd);
  
  u64 console = intern_string("ccconsole!");
  
  set_simple_graphics_control(console, id);
  set_color_alpha(console, vec4_new(1,1,1,0.3));
  set_console_height(console, 300);
  create_console(console);
  add_control(id, console);
  set_margin(console, (thickness){5,5,5,5});
  set_vertical_alignment(console, VALIGN_BOTTOM);
  set_console_history_cnt(console, 100);
  define_method(console, console_command_entered_method, (method)command_entered);
  define_method(console, key_handler_method, (method) console_handle_key);
  set_game_window(id, win_id);
  
  u64 game = intern_string("ggame!");
  define_method(game, render_control_method, (method)simple_grid_render);
  define_method(game, measure_control_method, (method)simple_grid_game_measure);
  define_method(game, key_handler_method, (method) game_handle_key);
  define_method(game, mouse_down_method, (method) simple_grid_game_mouse_down_func);
  define_method(game, mouse_over_method, (method)simple_grid_game_mouse_over_func);
  set_alternative_control(id, game);
  set_alternative_control(game, id);
  set_game_window(game, win_id);
  
  //if(once(game)){
    game_data _gd = get_simple_game_data(game);
    _gd.zoom = 0.4;
    set_simple_game_data(game, _gd);
    //}
  
  if(gui_get_control(win_id, id) == NULL  && gui_get_control(win_id, game) == NULL){
    add_control(win_id, id);
    set_focused_element(win_id, console);
  }

  
  if(false && once(console)){
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
  graphics_context gd;
  graphics_context_load(&gd);
  set_graphics_context(item, gd);
  
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


