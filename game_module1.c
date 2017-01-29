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

bool nth_parse_u32(char * commands, u32 idx, u32 * result){
  static char buffer[30];
  if(copy_nth(commands, idx, buffer, sizeof(buffer))){
    if(sscanf(buffer, "%i", result))
      return true;
  }
  return false;
}

bool nth_parse_f32(char * commands, u32 idx, f32 * result){
  static char buffer[30];
  if(copy_nth(commands, idx, buffer, sizeof(buffer))){
    if(sscanf(buffer, "%f", result))
      return true;
  }
  return false;
}

bool nth_str_cmp(char * commands, u32 idx, const char * comp){
  static char buffer[100];
  if(copy_nth(commands, idx, buffer, sizeof(buffer))) {
    return strcmp(buffer, comp) == 0;
  }
  return false;
}

CREATE_TABLE_DECL2(door_status, u32, bool);
CREATE_TABLE2(door_status, u32, bool);

typedef struct{
  u32 closed;
  u32 open;
}door_models;

CREATE_TABLE_DECL2(door_model, u32, door_models);
CREATE_TABLE2(door_model, u32, door_models);

interaction_status door_open(graphics_context * ctx, u32 interactor, u32 interactee, bool check_avail){
  UNUSED(interactor);
  bool is_open = false;
  if(!try_get_door_status(interactee, &is_open)){
    return INTERACTION_ERROR;
  }
  if(!check_avail)
    set_door_status(interactee, !is_open);
  door_models models = get_door_model(interactee);

  u32 model = !is_open ? models.closed : models.open;
  
  ((entity_data * )index_table_lookup(ctx->entities, interactee))->model = model;
  return INTERACTION_DONE;
}

bool set_door(graphics_context * gctx,  editor_context * ctx, char * commands){
  char part1[16] = {0};
  if(copy_nth(commands, 0, part1, sizeof(part1))
     && strcmp(part1, "set_door") == 0)
    {
      
      logd("??\n");
      bool status = false;
      if(copy_nth(commands, 1, part1, sizeof(part1))){
	if(strcmp(part1, "model") == 0){
	  u32 model = 0;
	  if(copy_nth(commands, 2, part1, sizeof(part1))){
	    sscanf(part1, "%i", &model);
	    if(model > 0){
	      bool status = get_door_status(ctx->selected_index);
	      door_models models = get_door_model(ctx->selected_index);
	      if(status){
		models.open = model;
	      }else
		models.closed = model;
	      set_door_model(ctx->selected_index, models);
	    }
	  }

	  return true;
	}
	int s = 0;
	sscanf(part1, "%i", &s);
	status = s;
	
      }

      if(ctx->selection_kind == SELECTED_ENTITY && ctx->selected_index != 0){

	set_door_status(ctx->selected_index, status);
	door_models models = get_door_model(ctx->selected_index);

	if(status){
	  if(models.open == 0)
	    models.open = ((entity_data * )index_table_lookup(gctx->entities, ctx->selected_index))->model;
	}else{
	  if(models.closed == 0)
	    models.closed = ((entity_data * )index_table_lookup(gctx->entities, ctx->selected_index))->model;
	}
	set_door_model(ctx->selected_index, models);
	((entity_data * )index_table_lookup(gctx->entities, ctx->selected_index))->model = status? models.open : models.closed;	
      
      }
      return true;
    }
  return false;
}

void door_update(graphics_context * ctx){
  UNUSED(ctx);
  //logd("Door update..\n");
}

typedef struct{
  u32 polygon1, polygon2;
}visctrl;

CREATE_TABLE_DECL2(vision_control, u32, visctrl);
CREATE_TABLE2(vision_control, u32, visctrl);
CREATE_TABLE_DECL2(vision_controlled, u32, u32);
CREATE_MULTI_TABLE2(vision_controlled, u32, u32);


bool enterctrl_edit(graphics_context * gctx, editor_context * ctx, char * commands){
  UNUSED(gctx);
  UNUSED(ctx);
  char part1[30];
  if(copy_nth(commands, 0, part1, sizeof(part1))){
    
    if(strcmp(part1, "create_visctrl") == 0 && ctx->selection_kind == SELECTED_ENTITY && ctx->selected_index != 0){
      entity_data * ed = index_table_lookup(gctx->entities, ctx->selected_index);
      u32 model = ed->model;
      model_data * md = index_table_lookup(gctx->models, model);
      
      
      char poly1buf[10];
      char poly2buf[10];
      if(copy_nth(commands, 1, poly1buf, sizeof(poly1buf))
	 && copy_nth(commands, 2, poly2buf, sizeof(poly2buf))){
	
	u32 i1[2] = {0};
	char * bufs[] = {poly1buf, poly2buf};
	index_table_sequence seq = md->polygons;
	for(u32 i = 0; i < 2; i++){
	  sscanf(bufs[i], "%i", &(i1[i]));
	  if(i1[i] < seq.index || i1[i] >= seq.index + seq.count){
	    logd("Invalid polygon..\n");
	    return true;
	  }
	  i1[i] = ((polygon_data *)index_table_lookup(gctx->polygon, i1[i]))->material;
	}
	
	set_vision_control(ctx->selected_index, (visctrl){.polygon1 = i1[0], .polygon2 = i1[1]});
      }else{
	logd("Fail..\n");
	return true;
      }
      logd("Create visctrl\n");
      
      
      return true;
    }else if(strcmp(part1, "set_visctrl") == 0){
      u32 control = 0, controlled = 0;
      char poly1buf[10];
      char poly2buf[10];
      if(copy_nth(commands, 1, poly1buf, sizeof(poly1buf))
	 && copy_nth(commands, 2, poly2buf, sizeof(poly2buf))){
	sscanf(poly1buf, "%i", &control);
	sscanf(poly2buf, "%i", &controlled);
	if(control == 0 || controlled == 0){
	  logd("Fail..\n");
	  return true;
	}
	polygon_data * pd = index_table_lookup(gctx->polygon, controlled);
	{
	  visctrl dummy;
	  vec4 dummy2;


	  
	  if(false == try_get_vision_control(control, &dummy)
	     || pd == NULL 
	     || false ==polygon_color_try_get(gctx->poly_color, pd->material, &dummy2)){
	    logd("Fail2\n");
	    return true;
	  }
	  
	}
	set_vision_controlled(control, pd->material);

	logd("Set vision control %i %i\n", control, controlled);
	return true;
      }	


    }else if(strcmp(part1, "list_visctrl") == 0){
      u32 entities[10];
      u32 controlled[10];
      visctrl vis[10];
      u64 idx2 = 0;
      u32 cnt = 0;
      logd("listing vision control items\n");
      while((cnt = iter_all_vision_control(entities, vis, array_count(vis), &idx2))){
	for(u32 i = 0; i < cnt; i++){
	  logd("%i:  %i %i\n", entities[i], vis[i].polygon1, vis[i].polygon2);
	}
      }

      idx2 = 0;
      logd("Listing vision controlled items\n");
      while((cnt = iter_all_vision_controlled(entities, controlled, array_count(controlled), &idx2))){
	for(u32 i = 0; i < cnt; i++){
	  logd("%i: %i %i\n", i, entities[i], controlled[i]);
	}
      }
      
      return true;
    }
    
  }
  return false;
}


// selected unit is actually the player unit.
CREATE_TABLE_DECL2(selected_unit, u32, bool);
CREATE_MULTI_TABLE2(selected_unit, u32, bool);

CREATE_TABLE_DECL2(material_heat, u32, float);
CREATE_TABLE2(material_heat, u32, float);

CREATE_TABLE_DECL2(heated_entity, u32, bool);
CREATE_TABLE2(heated_entity, u32, bool);
bool game1_set_selected_units(graphics_context * gctx, editor_context * ctx, char * commands){
  UNUSED(gctx);UNUSED(ctx);
  clear_selected_unit();
  if(nth_str_cmp(commands, 0, "set_selected")){
    for(u32 i = 1;true;i++){
      u32 v;
      if(nth_parse_u32(commands, i, &v)){
	set_selected_unit(v, true);
	logd("Selecting %i\n", v);
      }else{
	break;
      }
    }
    return true;
  }
  
  if(nth_str_cmp(commands, 0, "set_heat")){
    u32 polygon = 0;
    float newheat = 0;
    if(nth_parse_u32(commands, 1, &polygon)
       && nth_parse_f32(commands, 2, &newheat)){
      ASSERT(index_table_contains(gctx->polygon, polygon));
      polygon_data * pd = index_table_lookup(gctx->polygon, polygon);
      set_material_heat(pd->material, newheat);
    }
    return true;
  }
  
  if(nth_str_cmp(commands, 0, "heated")){
    u32 entity = 0;
    if(nth_parse_u32(commands, 1, &entity)){
      bool is = false;
      ASSERT(index_table_contains(gctx->entities, entity));
      if(try_get_heated_entity(entity, &is)){
	  unset_heated_entity(entity);
      }else{
	logd("Setting heated: %i\n", entity);
	set_heated_entity(entity, true);
      }
    }
    return true;
  }
  
  return false;
}


CREATE_TABLE_DECL2(hit_queue, u32, u32);
CREATE_TABLE2(hit_queue, u32, u32);

CREATE_TABLE_DECL2(entity_temperature, u32, float);
CREATE_TABLE2(entity_temperature, u32, float);
const float ambient_heat = -10;


void game1_interactions_update(graphics_context * ctx){
  static bool wasJoyActive = false;
  { // handle joystick


    
    int count;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);
    /*for(int i = 0; i < count; i++){
      logd("%f ", axes[i]);
      
    }
    logd("\n");*/

    u32 * selected_units = get_keys_selected_unit();
    u32 selected_unit_cnt = get_count_selected_unit();
    float x = axes[0];
    float y = -axes[1];
    if(x < 0.2 && x > - 0.2)
      x = 0;
    if(y < 0.2 && y > -0.2)
      y = 0;
    if(y != 0 || x != 0 || wasJoyActive){
      for(u32 i = 0; i < selected_unit_cnt; i++){
	entity_data * ed = index_table_lookup(ctx->entities, selected_units[i]);
	set_entity_target(selected_units[i], vec3_new(ed->position.x + x, ed->position.y, ed->position.z + y));
	ctx->game_data->offset.x += x * 0.01;
	ctx->game_data->offset.y += y * 0.01;
      }
    }
    if(x == 0 && y == 0)
      wasJoyActive = false;
    else wasJoyActive = true;

    
  }

  
  float heat = ambient_heat;
  game_event * ge = get_values_game_event();
  u64 count = get_count_game_event();
  for(u32 i = 0; i < count; i++){
    if(ge[i].mouse_button.button != mouse_button_left)
      continue;
    u32 entities[10];
    bool status[10];
    u64 idx2 = 0;
    u32 cnt = iter_all_door_status(entities, status, array_count(status), &idx2);
    static index_table * tab = NULL;
    if(tab == NULL) tab = index_table_create(NULL, sizeof(entity_local_data));
    index_table_clear(tab);
    vec2 p = ge[i].mouse_button.game_position;
    simple_game_point_collision(*ctx, entities, cnt, ge[i].mouse_button.game_position, tab);
    u64 door_cnt = 0;
    entity_local_data * ed = index_table_all(tab, &door_cnt);
    
    u32 * selected_units = get_keys_selected_unit();
    u32 selected_unit_cnt = get_count_selected_unit();
    if(door_cnt > 0){
      for(u32 i = 0; i < selected_unit_cnt; i++){
	set_hit_queue(selected_units[i], ed[0].entity);
      }
    }else{
      for(u32 i = 0; i < selected_unit_cnt; i++){
	unset_hit_queue(selected_units[i]);
      }
    }
    
    for(u32 i = 0; i < selected_unit_cnt; i++){
      entity_data * ed = index_table_lookup(ctx->entities, selected_units[i]);
      set_entity_target(selected_units[i], vec3_new(p.x, ed->position.y, p.y));
    }
  }

  { // vision controllers
    { // reset all vision controlled materials.
      u32 * materials = get_values_vision_controlled();
      u32 vision_controlled_count = get_count_vision_controlled();
      for(u32 i = 0; i < vision_controlled_count; i++){
	u32 material = materials[i];
	vec4 color = vec4_zero;
	if(polygon_color_try_get(ctx->poly_color, material, &color)){
	  color.w = 1.0;
	polygon_color_set(ctx->poly_color, material, color);	
	}
      }
    }

    u32 * selected_units = get_keys_selected_unit();
    u32 selected_unit_cnt = get_count_selected_unit();

    u32 * vision_controls = get_keys_vision_control();
    u32 vision_control_cnt = get_count_vision_control();
    
    static index_table * coltab = NULL;
    if(coltab == NULL)
      coltab = index_table_create(NULL, sizeof(collision_data));
    index_table_clear(coltab);

    detect_collisions_one_way(*ctx, selected_units, selected_unit_cnt, vision_controls, vision_control_cnt, coltab);
    
    u64 cnt2 = 0;
    collision_data * cd = index_table_all(coltab, &cnt2);
    
    for(u32 i = 0; i < cnt2; i++){
      u32 controlled[10];
      u64 idx = 0;
      u32 cnt = iter2_vision_controlled(cd[i].entity2.entity, controlled, array_count(controlled), &idx);
      for(u32 i = 0; i < cnt; i++){
	u32 material = controlled[i];
	vec4 color = vec4_zero;
	if(polygon_color_try_get(ctx->poly_color, material, &color)){
	  color.w = 0.0;
	  polygon_color_set(ctx->poly_color, material, color);
	}
      }
    }

    
    {//heated:
      index_table_clear(coltab);
      u32 * heated = get_keys_heated_entity();
      u32  cnt = get_count_heated_entity();
      detect_collisions_one_way(*ctx, selected_units, selected_unit_cnt, heated, cnt, coltab);
      u64 cnt2 = 0;
      collision_data * cd = index_table_all(coltab, &cnt2);
      for(u32 i = 0; i < cnt2; i++){
	float thisheat = 0.0;
	if(try_get_material_heat(cd[i].entity2.material, &thisheat)){
	  heat = MAX(thisheat, heat);
	}
      }
      
      logd("Temperature: %f\n", heat);
      for(u32 i = 0; i < selected_unit_cnt; i++){
	float temp = 38;
	try_get_entity_temperature(selected_units[i], &temp);
	if(heat < 10){
	  float dh = heat - 10; 
	  temp = MAX(heat, temp + dh * 0.001);
	}else if(heat > 15){
	  float dh = heat - 15;
	  temp = MIN(38, temp + dh * 0.1);
	}
	if(temp == 38.0f)
	  unset_entity_temperature(selected_units[i]);
	else
	  set_entity_temperature(selected_units[i], temp);
	logd("Entity temperature: %f\n", temp);
      }

    }
    const bool render_temperature_bar = true;
    if(render_temperature_bar){
      static u32 * bar_entity = NULL;
      if(bar_entity == NULL){
	auto bar_entity_area = create_mem_area("bar_entity");
	logd("Loading bar entity..\n");
	bar_entity = bar_entity_area->ptr;
	if(bar_entity_area->size == 0 || bar_entity[0] == 0){
	  mem_area_realloc(bar_entity_area, sizeof(u32));
	  bar_entity = bar_entity_area->ptr;
	  
	  u32 e1 = bar_entity[0];
	  if(bar_entity[0] == 0){
	    e1 = index_table_alloc(ctx->entities);
	    bar_entity[0] = e1;
	  }
	  active_entities_set(ctx->active_entities, e1, true);
	  entity_data * e = index_table_lookup(ctx->entities, e1);
	  u32 m1 = index_table_alloc(ctx->models);
	  e->model = m1;
	  model_data * m = index_table_lookup(ctx->models, m1);
	  index_table_resize_sequence(ctx->polygon, &(m->polygons), 2);
	  logd("Reload..\n");
	  u32 p1 = m->polygons.index;
	  {
	    polygon_add_vertex2f(ctx, p1, vec2_new(0, 0));
	    polygon_add_vertex2f(ctx, p1, vec2_new(0, 0.05));
	    polygon_add_vertex2f(ctx, p1, vec2_new(0.1, 0));
	    polygon_add_vertex2f(ctx, p1, vec2_new(0.1, 0.05));

	    polygon_data * pd = index_table_lookup(ctx->polygon, p1);
	    if(pd->material == 0)
	      pd->material = get_unique_number();
	    polygon_color_set(ctx->poly_color, pd->material, vec4_new(0.4, 0.4, 1.0, 1.0));
	  }

	  u32 p2 = p1 + 1;
	  {
	    polygon_add_vertex2f(ctx, p2, vec2_new(0, 0));
	    polygon_add_vertex2f(ctx, p2, vec2_new(0, 0.05));
	    polygon_add_vertex2f(ctx, p2, vec2_new(0.1, 0));
	    polygon_add_vertex2f(ctx, p2, vec2_new(0.1, 0.05));
	    polygon_data * pd2 = index_table_lookup(ctx->polygon, p2);
	    if(pd2->material == 0)
	      pd2->material = get_unique_number();
	    polygon_color_set(ctx->poly_color, pd2->material, vec4_new(0.5, 0.5, 1.0, 1.0));
	  }
	}
      }

      entity_data * e = index_table_lookup(ctx->entities, *bar_entity);
      model_data * m = index_table_lookup(ctx->models, e->model);
      polygon_data * p = index_table_lookup(ctx->polygon, m->polygons.index + 1);
      vertex_data * v = index_table_lookup_sequence(ctx->vertex, p->vertexes);
      
      static float * orig_width = NULL;
      if(orig_width == NULL){
	auto orig_width_area = create_mem_area("bar_entity_orig_width");
	if(orig_width_area->size == 0){
	  mem_area_realloc(orig_width_area, sizeof(f32));
	  orig_width = orig_width_area->ptr;
	  *orig_width = v[2].position.x;
	}else{
	  orig_width = orig_width_area->ptr;
	}
      }
      *orig_width = 0.1;
      if(selected_unit_cnt > 0){
	float temp = 38.0;
	try_get_entity_temperature(selected_units[0], &temp);
	float scale = (temp + 10.0) / (38.0 + 10.0);
	logd("scale: %f\n", scale);
	logd("%f \n", *orig_width * scale);
	v[2].position.x = *orig_width * scale;
	v[3].position.x = *orig_width * scale;
	graphics_context_reload_polygon(*ctx, m->polygons.index + 1);
      }
      vec2 offset = ctx->game_data->offset;
      e->position = vec3_new(offset.x - 0.38, 10, offset.y - 10 + 0.37);

      //logd("BAR POSITION: %i", bar_entity[0]); vec3_print(e->position);logd("\n");
    }
  }

  { // update hit queue
    static index_table * coltab = NULL;
    if(coltab == NULL)
      coltab = index_table_create(NULL, sizeof(collision_data));
    index_table_clear(coltab);
    u32 hit_queue_cnt = get_count_hit_queue();
    u32 * entities = get_keys_hit_queue();
    u32 * targets = get_values_hit_queue();
    u64 cnt2 = 0;
    for(u32 i = 0; i < hit_queue_cnt; i++){


      detect_collisions_one_way(*ctx, entities + i, 1, targets + i, 1, coltab);

      u32 saved = cnt2;
      index_table_all(coltab, &cnt2);
      if(saved != cnt2){
	{ // toggle door
	  bool door_status;
	  if(try_get_door_status(targets[i], &door_status)){
	    door_status = !door_status;
	    set_door_status(targets[i], door_status);
	    door_models models = get_door_model(targets[i]);
	    u32 model = !door_status ? models.closed : models.open;
	    ((entity_data * )index_table_lookup(ctx->entities, targets[i]))->model = model;
	  }
	}
      }
    }

    u32 hits[cnt2];
    collision_data * cd = index_table_all(coltab, &cnt2);
    u32 j = 0;
    for(u32 i = 0; i < cnt2; i++){
      u32 e = cd[i].entity1.entity;
      if(j == 0 || (hits[j - 1] != e))
	hits[j++] = e;
    }
    remove_hit_queue(hits, j);
  }

}

void init_module(graphics_context * ctx){
  UNUSED(ctx);
  logd("Loaded my module!\n");
  u32 open_door_interaction = intern_string("mod1/open door");
  graphics_context_load_interaction(ctx, door_open, open_door_interaction);
  set_desc_text2(open_door_interaction, GROUP_INTERACTION, "open door");

  u32 set_door_cmd = intern_string("mod1/set door");
  simple_game_editor_load_func(ctx, set_door, set_door_cmd);
  set_desc_text2(set_door_cmd, GROUP_INTERACTION, "set door");

  graphics_context_load_update(ctx, door_update, intern_string("mod1/door_update"));
  graphics_context_load_update(ctx, game1_interactions_update, intern_string("mod1/interactions"));
  simple_game_editor_load_func(ctx, enterctrl_edit, intern_string("mod1/set_vis_ctrl"));
  simple_game_editor_load_func(ctx, game1_set_selected_units, intern_string("mod1/set_selected_units"));
}
