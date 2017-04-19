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
#include "u32_lookup.h"
#include "line_element.h"
#include "line_element.c"
#include "connected_nodes.h"
#include "connected_nodes.c"
#include "connection_entities.h"
#include "connection_entities.c"
#include "character_table.h"
#include "character_table.c"
u32_lookup * is_node_table;
u32_lookup * is_selected_table;

character_table * characters;
index_table * bezier_curve_table;
line_element * line_element_table;
connected_nodes * connected_nodes_table;
connection_entities * connection_entities_table;
void load_connection_entity(graphics_context * ctx, u32 entity, u32 entity2){
  if(entity > entity2)
    SWAP(entity, entity2);
  u64 connection_id = ((u64)entity) | ((u64)entity2) << 32;
  u32 ced = 0;
  if(connection_entities_try_get(connection_entities_table, &connection_id, &ced) == false){
    ced = index_table_alloc(ctx->entities);
    connection_entities_set(connection_entities_table, connection_id, ced);
    active_entities_set(ctx->active_entities, ced, true);
  }
  logd("load connection entity %i %i %i\n", entity, entity2, ced);
  entity_data * ed = index_table_lookup(ctx->entities, ced);
  entity_data * e1 = index_table_lookup(ctx->entities, entity);
  entity_data * e2 = index_table_lookup(ctx->entities, entity2);
  if(e1 == NULL || e2 == NULL)
    return;
  if(ed->model == 0){
    ed->model = index_table_alloc(ctx->models);
  }
  model_data * md = index_table_lookup(ctx->models, ed->model);
  if(md->polygons.index == 0){
    md->polygons = index_table_alloc_sequence(ctx->polygon, 1);
  }
  polygon_data * pd = index_table_lookup_sequence(ctx->polygon, md->polygons);
  if(pd->vertexes.index == 0){
    index_table_resize_sequence(ctx->vertex, &(pd->vertexes), 4);
  }
  if(pd->material == 0){
    pd->material = get_unique_number();
    polygon_color_set(ctx->poly_color, pd->material, vec4_new(0,0,0,1));
  }
  vertex_data * vd = index_table_lookup_sequence(ctx->vertex, pd->vertexes);
  vec2 p1 = vec2_new(e1->position.x, e1->position.z);;
  vec2 p2 = vec2_new(e2->position.x, e2->position.z);;
  vec2 pdd = vec2_sub(p2, p1);
  vec2 pn = vec2_normalize(pdd);
  pn = vec2_new(pn.y * 0.02, -pn.x * 0.02);
  vec2 pn2 = vec2_scale(pn, -1);
  vd[0].position = vec2_add(p1, pn);
  vd[1].position = vec2_add(p1, pn2);
  vd[2].position = vec2_add(p2, pn);
  vd[3].position = vec2_add(p2, pn2);
  for(int i = 0; i < 4; i++){
    vec2_print(vd[i].position);
    logd(" ");
  }
  logd("\n");
  //graphics_context_reload_polygon(*ctx, ced);
}

bool node_roguelike_interact(graphics_context * gctx, editor_context * ctx, char * commands){
  UNUSED(gctx);
  u32 entity = 0;
  if(nth_parse_u32(commands, 1, &entity) == false && ctx->selection_kind == SELECTED_ENTITY)
    entity = ctx->selected_index;
  if(nth_str_cmp(commands, 0, "set-node")){
    if(u32_lookup_try_get(is_node_table, &entity)){
      u32_lookup_unset(is_node_table, entity);
      logd("Entity %i is not node.\n", entity);
    }
    else{
      u32_lookup_set(is_node_table, entity);
      logd("Entity %i is node.\n", entity);
    }
    return true;
  }
  if(nth_str_cmp(commands, 0, "node-list")){
    u32_lookup_print(is_node_table);
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
    load_connection_entity(gctx, entity, entity2);
    
    return true;
  }

  if(nth_str_cmp(commands, 0, "is-character")){
    u32 entity = ctx->selected_index;
    if(ctx->selection_kind != SELECTED_ENTITY || ctx->selected_index == 0){
      logd("Invalid entity\n");
    }else if(u32_lookup_try_get(is_node_table, &entity)){
      logd("A node cannot be a character\n");
    }else{

      u32 node;
      if(character_table_try_get(characters, &entity, &node)){
	character_table_unset(characters, entity);
	logd("Entity %i is not character.\n", entity);
      }
      else{
	u32 node = 0;
	if(nth_parse_u32(commands, 1, &node) == false || node == 0 || !u32_lookup_try_get(is_node_table, &node)){
	  logd("Second argument must be a node!\n");
	}else{
	  character_table_set(characters, entity, node);
	  logd("Entity %i is character.\n", entity);
	}
      }
    }
    return true;
  }
  
  if(nth_str_cmp(commands, 0, "is-selected")){
    u32 cnt = 0;
    u32 buf = 0;
    while(nth_parse_u32(commands, 1 + cnt, &buf)) cnt++;
    u32 items[cnt];
    for(u32 i = 0; i < cnt; i++){
      if(!nth_parse_u32(commands, 1 + i, items + i) || items[i] == 0){
	logd("item #%i is not an integer bigget than 0.");
	return true;
      }
    }
    int cmpfnc(u32 * a, u32 * b){
      if(*a > *b)
	return 1;
      else if(*a < *b)
	return -1;
      return 0;
    }
    qsort(items, cnt, sizeof(u32), (void *) cmpfnc);
    for(u32 i = 0; i < cnt; i++){
      logd("selecting: %i: %i\n", i + 1, items[i]);
    }
    u32_lookup_clear(is_selected_table);
    u32_lookup_insert(is_selected_table, items, cnt);
    u32_lookup_print(is_selected_table);
    return true;
    }
  
  return false;
}

void node_roguelike_update(graphics_context * ctx){
  UNUSED(ctx);
  //u32_lookup_print(is_node_table);
  u32 cnt = ctx->game_event_table->count;
  if(cnt > 0){

    game_event * events = ctx->game_event_table->event + 1;
    for(u32 i = 0; i < cnt; i++){
      game_event evt = events[i];
      if(evt.mouse_button.button == mouse_button_left && evt.mouse_button.pressed){
	static index_table * tab = NULL;
	if(tab == NULL) tab = index_table_create(NULL, sizeof(entity_local_data));
	index_table_clear(tab);
	vec2 pt = evt.mouse_button.game_position;
	simple_game_point_collision(*ctx, is_node_table->key + 1, is_node_table->count, pt, tab);
	u64 hitcnt = 0;
	u32 * e = index_table_all(tab, &hitcnt);
	if(hitcnt > 0){
	  u64 indexes[is_selected_table->count];
	  character_table_lookup(characters, is_selected_table->key + 1, indexes, is_selected_table->count);
	  for(u32 i = 0; i < is_selected_table->count; i++){
	    characters->node[indexes[i]] = e[0];
	  }
	}
      }
    }
  }

  for(u32 i = 0; i < characters->count; i++){
    u32 entity = characters->entity[i + 1];
    u32 nodeid = characters->node[i + 1];
    entity_data * ed = index_table_lookup(ctx->entities, entity);
    entity_data * node = index_table_lookup(ctx->entities, nodeid);
    ASSERT(ed != NULL);
    ASSERT(node != NULL);
    vec3 n2c = vec3_sub(node->position, ed->position);
    auto len = vec3_len(n2c);
    if(len > 0.2){

      vec3 d = vec3_scale(n2c, 0.3 * 1.0 / len);
      entity_velocity_set(ctx->entity_velocity, entity, d);
    }else{
      entity_velocity_unset(ctx->entity_velocity, entity);
    } 
  }
  
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
  is_node_table = u32_lookup_create("is_node");
  characters = character_table_create("characters");
  bezier_curve_table = index_table_create("bezier", sizeof(vec2));
  line_element_table = line_element_create("line_elements");
  is_selected_table = u32_lookup_create("selected-entities");
  connected_nodes_table = connected_nodes_create("connected_nodes");
  connection_entities_table = connection_entities_create("connection_entities");
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
