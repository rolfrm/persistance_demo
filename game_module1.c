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

bool nth_parse_u32(char * commands, u32 idx, u32 * result){
  static char buffer[30];
  if(copy_nth(commands, idx, buffer, sizeof(buffer))){
    if(sscanf(buffer, "%i", result))
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

// selected unit is actually the player unit.
CREATE_TABLE_DECL2(selected_unit, u32, bool);
CREATE_MULTI_TABLE2(selected_unit, u32, bool);

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
  return false;
}

void game1_interactions_update(graphics_context * ctx){
  u32 * evt_key = get_keys_game_event();
  game_event * ge = get_values_game_event();
  u64 count = get_count_game_event();
  for(u32 i = 0; i < count; i++){
    logd("%i %f %f %i %i\n", evt_key[i], ge[i].mouse_button.game_position.x, ge[i].mouse_button.game_position.y, ge[i].mouse_button.button, count);
    u32 entities[10];
    bool status[10];
    u64 idx2 = 0;
    u32 cnt = iter_all_door_status(entities, status, array_count(status), &idx2);
    static index_table * tab = NULL;
    if(tab == NULL) tab = index_table_create(NULL, sizeof(entity_local_data));
    index_table_clear(tab);
    logd("Door count: %i\n", cnt);
    simple_game_point_collision(*ctx, entities, cnt, ge[i].mouse_button.game_position, tab);
    u64 door_cnt = 0;
    entity_local_data * ed = index_table_all(tab, &door_cnt);
    logd("Hit doors: %i\n", door_cnt);
    for(u32 i = 0; i < door_cnt; i++){
      logd("ed: %i\n", ed[i].entity);
    }
  }

  {
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
