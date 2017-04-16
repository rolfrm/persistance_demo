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
#include "line_element.h"
#include "line_element.c"
#include "connected_nodes.h"
#include "connected_nodes.c"

is_node * is_node_table;
index_table * bezier_curve_table;
line_element * line_element_table;
connected_nodes * connected_nodes_table;
bool node_roguelike_interact(graphics_context * gctx, editor_context * ctx, char * commands){
  UNUSED(gctx);
  u32 entity = 0;
  if(nth_parse_u32(commands, 1, &entity) == false && ctx->selection_kind == SELECTED_ENTITY)
    entity = ctx->selected_index;
  if(nth_str_cmp(commands, 0, "set_node")){
    if(is_node_try_get(is_node_table, &entity)){
      is_node_unset(is_node_table, entity);
      logd("Entity %i is not node.\n", entity);
    }
    else{
      is_node_set(is_node_table, entity);
      logd("Entity %i is node.\n", entity);

    }
    return true;
  }

  if(nth_str_cmp(commands, 0, "connect")){
    u32 entity2 = 0;
    if(nth_parse_u32(commands, 2, &entity2) == false && ctx->selection_kind == SELECTED_ENTITY){
      entity2 = ctx->selected_index;
      SWAP(entity, entity2);
    }
    
    if(entity2 == 0 || entity == 0){
      logd("Invalid entity selected\n");
      return true;
    }
    if(entity2 == entity){
      logd("A node cannot be connected to itself\n");
      return true;
    }

    u64 count = connected_nodes_iter(connected_nodes_table, &entity, 1, NULL, NULL, connected_nodes_table->count, NULL);
    connected_nodes_print(connected_nodes_table);
    logd("connections for %i: %i\n", entity, count);
    if(count > 1){
      u64 indexes[count];
      connected_nodes_iter(connected_nodes_table, &entity, 1, NULL, indexes, count, NULL);
      for(u32 i = 0; i < count; i++){
	if(connected_nodes_table->n2[indexes[i]] == entity2){
	  logd("Node %i and %i are already connected!\n", entity, entity2);
	  return true;
	}
      }
    }

    connected_nodes_set(connected_nodes_table, entity, entity2);
    connected_nodes_set(connected_nodes_table, entity2, entity);
    logd("Connected %i %i\n", entity2, entity);
    return true;
  }
  
  return false;
}

void node_roguelike_update(graphics_context * ctx){
  UNUSED(ctx);
  is_node_print(is_node_table);
}

void load_bezier_into_model(u32 model){
  index_table_sequence seq;
  ASSERT(line_element_try_get(line_element_table, &model, &seq, NULL));
  vec2 * data = index_table_lookup_sequence(bezier_curve_table, seq);
  vec2 p0 = data[0];
  vec2 p1 = data[1];
  vec2 p3 = data[2];
  vec2 p2 = data[3];

  logd("%f, %f\n", p0.x, p0.y);
  vec2 pp = p0;
  vec2 pt = vec2_new(0, 0);
  float acct = 0.0;
  float step = 0.1;
  float i = step;
  bool first = true;
  float detail = 0.001;
  for(;i<1.0; i += step){
    //iron_usleep(10000);
    //logd("%f %f %f\n", i, step, acct);
    float a = (1.0f - i);
    vec2 f0 = vec2_scale(p0, a * a * a);
    vec2 f1 = vec2_scale(p1, 3 * a * a * i);
    vec2 f2 = vec2_scale(p2, 3 * a * i * i);
    vec2 f3 = vec2_scale(p3, i * i * i);
    vec2 p = vec2_add(vec2_add(f0, f1), vec2_add(f2, f3));
    vec2 dp = vec2_sub(p, pp);
    vec2 tp = vec2_normalize(vec2_new(dp.y, dp.x));
    float dot = acos(vec2_mul_inner(tp, pt)) * 0.5;

    if(acct + dot > 0.1 && !first){
      i-= step;
      step = step * 0.5;
      //logd(">> %f %f\n", dot, i);
      continue;
    }
    pt = tp;
    pp = p;    
    if(!first)
      acct += dot;

    if(acct > detail * 0.6){
      logd("%f, %f,  %f, %f\n", p.x, p.y, acct, dot);
      acct = 0.0;
    }else{
      step *= 1.5;
    }
    first = false;
  }
}

void init_module(graphics_context * ctx){
  static bool is_initialized = false;
  if(is_initialized) return;
  is_initialized = true;
  
  logd("INIT node roguelike MODULE\n");
  is_node_table = is_node_create("is_node");
  bezier_curve_table = index_table_create("bezier", sizeof(vec2));
  line_element_table = line_element_create("line_elements");
  connected_nodes_table = connected_nodes_create("connected_nodes");
  ((bool *)&connected_nodes_table->is_multi_table)[0] = true;
  index_table_clear(bezier_curve_table);
  if(line_element_table->count == 0){
    // test bezier
    logd("loading bezier...\n");
    u32 key = 0xFFFFAA01;
    index_table_sequence seq;
    //if(line_element_try_get(line_element_table, &key, &seq, NULL))
    //   index_table_remove_sequence(bezier_curve_table, &seq);
    
    seq = index_table_alloc_sequence(bezier_curve_table, 4);
    
    line_element_set(line_element_table, key, seq, true);
      vec2 * data = index_table_lookup_sequence(bezier_curve_table, seq);
      data[0] = vec2_new(0,0);
      data[1] = vec2_new(0,0.5);
      data[2] = vec2_new(1,0);
      data[3] = vec2_new(1,-0.5);
      line_element_set(line_element_table, key, seq, false);
      load_bezier_into_model(key);
  }

  graphics_context_load_update(ctx, node_roguelike_update, intern_string("node_roguelike/interactions"));
  simple_game_editor_load_func(ctx, node_roguelike_interact, intern_string("node_roguelike/editor_interact"));
}
