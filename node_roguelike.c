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
#include "abstract_sortable.h"
#include "is_node.h"
#include "is_node.c"

is_node * is_node_table;

bool node_roguelike_interact(graphics_context * gctx, editor_context * ctx, char * commands){
  logd("Interact!\n");
  UNUSED(gctx);
  u32 entity = 0;
  if(nth_parse_u32(commands, 1, &entity) == false && ctx->selection_kind == SELECTED_ENTITY)
    entity = ctx->selected_index;
  if(nth_str_cmp(commands, 0, "set_node")){
    if(is_node_try_get(is_node_table, &entity)){
      is_node_unset(is_node_table, entity);
      logd("Unset entity: %i\n", entity);
    }
    else{
      is_node_set(is_node_table, entity);
      logd("Set entity: %i\n", entity);
    }
    return true;
  }
  return false;
}


void node_roguelike_update(graphics_context * ctx){
  UNUSED(ctx);
  logd("%i:", is_node_table->count);
  for(u32 i = 0; i < is_node_table->count; i++){
    logd(" %i ", is_node_table->index[i]);
  }
  logd("\n");

}

void init_module(graphics_context * ctx){
  logd("INIT node roguelike MODULE\n");
  is_node_table = is_node_create("is_node");
  graphics_context_load_update(ctx, node_roguelike_update, intern_string("node_roguelike/interactions"));
  simple_game_editor_load_func(ctx, node_roguelike_interact, intern_string("node_roguelike/editor_interact"));
}
