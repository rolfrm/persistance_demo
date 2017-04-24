#include <iron/full.h>
#include "persist.h"
#include "sortable.h"
#include "persist_oop.h"
#include "gui.h"
#include "index_table.h"

#include "abstract_sortable.h"
#include "u32_lookup.h"
#include "u32_to_u32.h"
#include "u32_to_u32.c"
#include "simple_graphics.h"
#include "line_element.h"
#include "line_element.c"
#include "connected_nodes.h"
#include "connected_nodes.c"
#include "connection_entities.h"
#include "connection_entities.c"
#include "character_table.h"
#include "character_table.c"
#include "u32_to_sequence.h"
#include "u32_to_sequence.c"
#include "node_roguelike.h"
#include "node_action_table.h"
#include "nr_ui_item.h"
#include "nr_ui_item.c"


typedef struct{
  float time_step;

}_nr_game_data;

extern node_action_table * node_actions;
void sort_u32(u32 * items, u64 cnt){
  int cmpfnc(u32 * a, u32 * b){
    if(*a > *b)
      return 1;
    else if(*a < *b)
      return -1;
    return 0;
  }
  qsort(items, cnt, sizeof(u32), (void *) cmpfnc);
}

u32_lookup * is_node_table;
u32_lookup * is_selected_table;
character_table * characters;
index_table * bezier_curve_table;
line_element * line_element_table;
connected_nodes * connected_nodes_table;
connection_entities * connection_entities_table;
_nr_game_data * nr_game_data;
index_table * node_traversal;
u32_to_sequence * node_traversal_index;
u32_to_u32 * node_traversal_offset;
u32_lookup * visited_nodes;
nr_ui_item * ui_items;
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
  vec2 p1 = vec2_new(e1->position.x, e1->position.z);
  vec2 p2 = vec2_new(e2->position.x, e2->position.z);
  vec2 pdd = vec2_sub(p2, p1);
  vec2 pn = vec2_normalize(pdd);
  pn = vec2_new(pn.y * 0.02, -pn.x * 0.02);
  vec2 pn2 = vec2_scale(pn, -1);
  vd[0].position = pn;
  vd[1].position = pn2;
  vd[2].position = vec2_add(pdd, pn);
  vd[3].position = vec2_add(pdd, pn2);
  ed->position = e1->position;
  graphics_context_reload_polygon(*ctx, ced);
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
    bool connect = true;
    if(count > 1){
      u64 indexes[count];
      connected_nodes_iter(connected_nodes_table, &entity, 1, NULL, indexes, count, NULL);
      for(u32 i = 0; i < count; i++){
	if(connected_nodes_table->n2[indexes[i]] == entity2){
	  logd("Node %i and %i are already connected!\n", entity, entity2);
	  connect = false;
	  break;
	  //return true;
	}
      }
    }
    if(connect){
      connected_nodes_set(connected_nodes_table, entity, entity2);
      connected_nodes_set(connected_nodes_table, entity2, entity);
    }
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
      u32 target;
      
      if(character_table_try_get(characters, &entity, &node, &target)){
	character_table_unset(characters, entity);
	logd("Entity %i is not character.\n", entity);
      }
      else{
	u32 node = 0;
	if(nth_parse_u32(commands, 1, &node) == false || node == 0 || !u32_lookup_try_get(is_node_table, &node)){
	  logd("Second argument must be a node!\n");
	}else{
	  character_table_set(characters, entity, node, 0);
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
    sort_u32(items, cnt);
    for(u32 i = 0; i < cnt; i++){
      logd("selecting: %i: %i\n", i + 1, items[i]);
    }
    u32_lookup_clear(is_selected_table);
    u32_lookup_insert(is_selected_table, items, cnt);
    u32_lookup_print(is_selected_table);
    return true;
    }
  if(nth_str_cmp(commands, 0, "nr-load-player")){
    if(ctx->selection_kind != SELECTED_ENTITY || ctx->selected_index == 0){
      logd("Invalid entity\n");
    }else{
      u32 entity = ctx->selected_index;
      while(u32_to_u32_try_get(ui_subnodes, &entity, NULL))
	u32_to_u32_remove(ui_subnodes, &entity, 1);
      
      {
	u32 nodeid = 0xFF01;
	u32_to_u32_set(ui_subnodes, entity, nodeid);
	u32_to_u32_set(ui_node_actions, nodeid, intern_string("inventory"));

	while(u32_to_u32_try_get(ui_subnodes, &nodeid, NULL))
	  u32_to_u32_remove(ui_subnodes, &nodeid, 1);
	{
	  
	  u32 nodeid2 = 0xFF04;
	  u32_to_u32_set(ui_subnodes, nodeid , nodeid2);
	  u32_to_u32_set(ui_node_actions, nodeid2, intern_string("apple"));
	}
	{
	  u32 nodeid2 = 0xFF05;
	  u32_to_u32_set(ui_subnodes, nodeid , nodeid2);
	  u32_to_u32_set(ui_node_actions, nodeid2, intern_string("orange"));
	}
	{
	  u32 nodeid2 = 0xFF06;
	  u32_to_u32_set(ui_subnodes, nodeid , nodeid2);
	  u32_to_u32_set(ui_node_actions, nodeid2, intern_string("pear"));
	}
	
      }
      {
	u32 nodeid = 0xFF02;
	u32_to_u32_set(ui_subnodes, entity, nodeid);
	u32_to_u32_set(ui_node_actions, nodeid, intern_string("stats"));
      }

      {
	u32 nodeid = 0xFF03;
	u32_to_u32_set(ui_subnodes, entity, nodeid);
	u32_to_u32_set(ui_node_actions, nodeid, intern_string("look"));
      }
      
      u32_to_u32_print(ui_node_actions);

    }
    return true;
  }

  if(nth_str_cmp(commands, 0, "nr-traverse")){
    if(ctx->selection_kind != SELECTED_ENTITY || ctx->selected_index == 0){
      logd("An entity must be selected for nr-traverse\n");
      return true;
    }
    
    u32_to_u32_unset(node_traversal_offset, ctx->selected_index);
    u32 count = 0;
    {
      u32 node;
      while(nth_parse_u32(commands, 1 + count, &node)){count++;}
    }
    index_table_sequence seq = {0};
    u32_to_sequence_try_get(node_traversal_index, &ctx->selected_index, &seq);
    index_table_resize_sequence(node_traversal, &seq, count);
    u32 * nodes = index_table_lookup_sequence(node_traversal, seq);;
    for(u32 i = 0; i < count; i++){
      u32 node = 0;
      nth_parse_u32(commands, 1 + i, &node);
      nodes[i] = node;
    }
    u32_to_sequence_set(node_traversal_index, ctx->selected_index, seq);
    return true;
  }

  if(nth_str_cmp(commands, 0, "nr-load-ui")){
    u32 entity = 0;
    if(!nth_parse_u32(commands, 1, &entity) || entity == 0){
      logd("valid entity must be selected!\n");
      return true;
    }
    u32_to_u32_remove(ui_subnodes, &entity, 1);
    {
      u32 nodeid = 0xFFA01;
      u32_to_u32_set(ui_subnodes, entity, nodeid);
      u32_to_u32_set(ui_node_actions, nodeid, intern_string("pause"));	
    }
    {
      u32 nodeid = 0xFFA02;
      u32_to_u32_set(ui_subnodes, entity, nodeid);
      u32_to_u32_set(ui_node_actions, nodeid, intern_string(">"));
    }
    
    {
      u32 nodeid = 0xFFA03;
      u32_to_u32_set(ui_subnodes, entity, nodeid);
      u32_to_u32_set(ui_node_actions, nodeid, intern_string(">>"));
    }

    {
      u32 nodeid = 0xFFA04;
      u32_to_u32_set(ui_subnodes, entity, nodeid);
      u32_to_u32_set(ui_node_actions, nodeid, intern_string(">>>"));
    }
    node_roguelike_ui_show(entity, entity);
    return true;
  }

  if(nth_str_cmp(commands, 0, "make-ui")){
    if(ctx->selection_kind != SELECTED_ENTITY || ctx->selected_index == 0){
      logd("An entity must be selected for make-ui\n");
      return true;
    }
    vec2 offset = {0};
    nth_parse_f32(commands, 1, &offset.x);
    nth_parse_f32(commands, 2, &offset.y);
    nr_ui_item_set(ui_items, ctx->selected_index, offset);
    return true;
  }
  
  return false;
}

void inventory_action(u32 node){
  u64 subnode_idx = 0;
  u64 cnt = u32_to_u32_iter(ui_subnodes, &node, 1, NULL, &subnode_idx, 1, NULL);
  if(cnt > 0 && u32_lookup_try_get(shown_ui_nodes, &ui_subnodes->value[subnode_idx])){
    node_roguelike_ui_hide_subnodes(node);      
  }else{
    node_roguelike_ui_show(node, 0);      
  }
}

double game_time = 0;
void node_roguelike_update(graphics_context * ctx){
  ctx->game_data->time_step = nr_game_data->time_step;
  u32 cnt = ctx->game_event_table->count;
  game_time += ctx->game_data->time_step * 60;
  int seconds = game_time;
  int minutes = seconds / 60;
  int hours = minutes / 60;
  UNUSED(hours);
  //logd("Game time: %02i:%02i:%02i\n", hours % 24, minutes % 60, seconds % 60);
  if(cnt > 0){

    game_event * events = ctx->game_event_table->event + 1;
    for(u32 i = 0; i < cnt; i++){
      game_event evt = events[i];
      //logd("Event count: %i: %i %i %i\n", evt.key.key,  key_space, key_enter, key_release);
      if(evt.kind == GAME_EVENT_KEY){
	if(evt.key.key == key_space && evt.key.action == key_press){
	  if(ctx->game_data->time_step < 0.0001f){
	    logd("Unpause\n");
	    ctx->game_data->time_step = 1.0f / 60.0f;
	  }else{
	    ctx->game_data->time_step = 0.0f;
	    logd("Pause\n");
	  }
	}
      }
      
      if(evt.kind == GAME_EVENT_MOUSE_BUTTON && evt.mouse_button.button == mouse_button_left && evt.mouse_button.pressed){
	vec2 pt = evt.mouse_button.game_position;
	
	static index_table * tab = NULL;
	if(tab == NULL) tab = index_table_create(NULL, sizeof(entity_local_data));
	index_table_clear(tab);
	{ // check if ui nodes are clicked
	  u64 node_entities_index[shown_ui_nodes->count];
	  u32_to_u32_lookup(node_to_entity, shown_ui_nodes->key + 1, node_entities_index, shown_ui_nodes->count);
	  u32 entities[shown_ui_nodes->count];
	  for(u64 i = 0; i < array_count(entities); i++)
	    entities[i] = node_to_entity->value[node_entities_index[i]];
	  sort_u32(entities, array_count(entities));
	  simple_game_point_collision(*ctx, entities, array_count(entities), pt, tab);
	  u64 hitcnt = 0;
	  u32 * e = index_table_all(tab, &hitcnt);
	  if(hitcnt > 0){
	    for(u32 i = 0; i < node_to_entity->count; i++){
	      u32 _e = node_to_entity->value[i + 1];
	      u32 node = node_to_entity->key[i + 1];
	      if(_e == e[0]){
		u32 node_action = 0;
		if(u32_to_u32_try_get(ui_node_actions, &node, &node_action) ){
		  ui_node_action action = NULL;
		  if(node_action_table_try_get(node_actions, &node_action, &action) && action != NULL)
		    action(node);
		}
		
		break;
	      }
	    }
	    
	    goto next_evt;
	  }
	}
	
	{ // check if characters are clicked.
	  simple_game_point_collision(*ctx, characters->entity + 1, characters->count, pt, tab);
	  u64 hitcnt = 0;
	  u32 * e = index_table_all(tab, &hitcnt);
	  for(u32 i = 0; i < hitcnt; i++){
	    logd("Character %i clicked\n", e[i]);
	  }
	  
	  if(hitcnt > 0){
	    u32 entity = e[0];
	    logd("SHOW: %i\n", entity);
	    //entity_data * ed = index_table_lookup(ctx->entities,
  //entity);
	    inventory_action(entity);
	    //node_roguelike_ui_show(entity, entity);
	    goto next_evt;
	  }	  
	}
	
	{ // check if node is clicked
	  simple_game_point_collision(*ctx, is_node_table->key + 1, is_node_table->count, pt, tab);
	  u64 hitcnt = 0;
	  u32 * e = index_table_all(tab, &hitcnt);
	  if(hitcnt > 0){
	    u64 indexes[is_selected_table->count];
	    character_table_lookup(characters, is_selected_table->key + 1, indexes, is_selected_table->count);
	    for(u32 i = 0; i < is_selected_table->count; i++){
	      characters->target_node[indexes[i]] = e[0];
	    }
	  }
	}
      }
    next_evt:;
    }
  }

  u32 current_node = 0;
  for(u32 i = 0; i < is_selected_table->count; i++){
    u32 entity = is_selected_table->key[i + 1];
    u32 node;
    u32 target;
    if(character_table_try_get(characters, &entity, &node, &target)){
      current_node = node;
      u32_lookup_set(visited_nodes, node);
      u32 connection_count = connected_nodes_iter(connected_nodes_table, &node, 1, NULL, NULL, 100, NULL);
      u64 indexes[connection_count];
      connected_nodes_iter(connected_nodes_table, &node, 1, NULL, indexes, connection_count, NULL);
      for(u32 i = 0; i < connection_count; i++){
	u64 index = indexes[i];
	if(index != 0)
	  u32_lookup_set(visited_nodes, connected_nodes_table->n2[index]);
      }
    }
  }

  u32_lookup_clear(ctx->game_visible);
  if(current_node != 0){
    for(u32 i = 0; i < characters->count; i++){
      if(characters->current_node[i + 1] == current_node || characters->target_node[i + 1] == current_node){
	u32 entity = characters->entity[1 + i];
	u32_lookup_insert(ctx->game_visible, &entity, 1);
      }
    }
  }
  u32_lookup_insert(ctx->game_visible, visited_nodes->key + 1, visited_nodes->count);

  { // add UI nodes
    u64 ui_node_entities_index[shown_ui_nodes->count];
    u32_to_u32_lookup(node_to_entity, shown_ui_nodes->key + 1, ui_node_entities_index, shown_ui_nodes->count);
    u32 cnt = 0;
    for(u32 i = 0; i < shown_ui_nodes->count; i++){
      if(ui_node_entities_index[i] != 0)
	cnt++;
    }
    u32 entities[cnt];
    u32 j = 0;
    for(u32 i = 0; i < shown_ui_nodes->count; i++){
      u64 index = ui_node_entities_index[i];
      if(index != 0){
	entities[j] = node_to_entity->value[index];
	j++;
      }
    }
    sort_u32(entities, cnt);
    u32_lookup_insert(ctx->game_visible, entities, cnt);
  }
 
  u32_lookup_insert(ctx->game_visible, shown_ui_nodes->key + 1, shown_ui_nodes->count);
  for(u32 i = 0; i < characters->count; i++){
    u32 entity = characters->entity[i + 1];
    u32 nodeid = characters->target_node[i + 1];
    if(nodeid == 0){
      index_table_sequence seq = {0};
      // check if there is a number of nodes in the traversal list.
      if(u32_to_sequence_try_get(node_traversal_index, &entity, &seq) && seq.count > 0){
	u32 * traversal_list = index_table_lookup_sequence(node_traversal, seq);
	characters->target_node[i + 1] = traversal_list[0];
      }
      continue;
    }

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
      index_table_sequence seq = {0};
      // check if there is a number of nodes in the traversal list.
      if(u32_to_sequence_try_get(node_traversal_index, &entity, &seq)){

	u32 * traversal_list = index_table_lookup_sequence(node_traversal, seq);
	
	u32 offset = 0;	
	u32_to_u32_try_get(node_traversal_offset, &entity, &offset);
	offset += 1;
	if(offset >= seq.count)
	  offset = 0;
	ASSERT(traversal_list[offset] != 0);
	characters->current_node[i + 1] = characters->target_node[i + 1];
	characters->target_node[i + 1] = traversal_list[offset];
	u32_to_u32_set(node_traversal_offset, entity, offset);
      }
      characters->current_node[i + 1] = nodeid;
    } 
  }

  for(u32 i = 0; i < ui_items->count; i++){
    u32 item = ui_items->key[i + 1];
    vec2 offset = ui_items->position[i + 1];
    entity_data * ed = index_table_lookup(ctx->entities, item);
    ASSERT(ed != NULL);
    vec2 game_offset = ctx->game_data->offset;
    float zoom = ctx->game_data->zoom;
    ed->position = vec3_new(game_offset.x - offset.x * zoom, 10, game_offset.y - 10 + offset.y * zoom);
  }
  
  
  node_roguelike_ui_update(ctx);
  
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

void look_action(u32 node){
  logd("Look around: %i\n", node);
}

void stats_action(u32 node){
  logd("Check stats: %i\n", node);
}

void nr_pause(u32 node){
  nr_game_data->time_step = 0.0f;
  logd("Pause %i\n", node);
}

void nr_normal_speed(u32 node){
  nr_game_data->time_step = 2.0 / 60.0;
  logd("Normal Speed %i\n", node);
}

void nr_fast_speed(u32 node){
  nr_game_data->time_step = 10.0 / 60.0;
  logd("Fast Speed %i\n", node);
}

void nr_ultra_speed(u32 node){
  nr_game_data->time_step = 100.0 / 60.0;
  logd("Ultra Speed %i\n", node);
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
  ((bool *)&connected_nodes_table->is_multi_table)[0] = true;
  ui_items = nr_ui_item_create("nr-ui-item");
  connection_entities_table = connection_entities_create("connection_entities");
  
  node_traversal_offset = u32_to_u32_create("node-traversal-offset");
  node_traversal_index = u32_to_sequence_create("node-traversal-index");
  node_traversal = index_table_create("node-traversal", sizeof(u32));
  if(ctx->game_visible == NULL){
    ctx->game_visible = u32_lookup_create("game-visible");
  }
  visited_nodes = u32_lookup_create("visited-nodes");
  static mem_area * game_data_mem_area = NULL;
  if(game_data_mem_area == NULL){
    game_data_mem_area = create_mem_area("nr-game-data");
    if(game_data_mem_area->size != sizeof(_nr_game_data))
      mem_area_realloc(game_data_mem_area, sizeof(_nr_game_data));
    nr_game_data = game_data_mem_area->ptr;
  }

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
  node_roguelike_ui_init();


  // must be loaded each run.
  node_roguelike_ui_register(intern_string("inventory"), inventory_action);
  node_roguelike_ui_register(intern_string("look"), look_action);
  node_roguelike_ui_register(intern_string("stats"), stats_action);
  node_roguelike_ui_register(intern_string("pause"), nr_pause);
  node_roguelike_ui_register(intern_string(">"), nr_normal_speed);
  node_roguelike_ui_register(intern_string(">>"), nr_fast_speed);
  node_roguelike_ui_register(intern_string(">>>"), nr_ultra_speed); 
}
