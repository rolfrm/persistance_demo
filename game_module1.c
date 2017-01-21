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


void init_module(graphics_context * ctx){
  UNUSED(ctx);
  logd("Loaded my module!\n");
  u32 open_door_interaction = intern_string("mod1/open door");
  graphics_context_load_interaction(ctx, door_open, open_door_interaction);
  set_desc_text2(open_door_interaction, GROUP_INTERACTION, "open door");

  u32 set_door_cmd = intern_string("mod1/set door");
  simple_game_editor_load_func(ctx, set_door, set_door_cmd);
  set_desc_text2(set_door_cmd, GROUP_INTERACTION, "set door");
}
