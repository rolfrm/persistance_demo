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
#include "u32_to_f32.h"
#include "u32_to_f32.c"
#include "node_roguelike.h"
#include "node_action_table.h"
#include "nr_ui_item.h"
#include "nr_ui_item.c"

typedef struct __attribute__ ((packed)){
  u8 color1:4; // body color
  u8 color2:4; // stripe color
  u8 color3:4; // circ color
  u8 has_circle:1;
  u8 has_stripe:1;
  i8 circle_offset:3;
  i8 stripe_offset:3;
}people_dna;

people_dna sanitize_dna(people_dna dna){
  if(dna.has_circle == false || dna.color3 == dna.color1){
    dna.circle_offset = 0;
    dna.color3 = 0;
  }
  if(dna.has_stripe == false){
    dna.stripe_offset = 0;
    dna.color2 = 0;
  }
  if(dna.has_stripe == false && dna.has_circle == false)
    dna.color2 = 0;
  return dna;
}

u32 people_dna_to_bit(people_dna dna){
  dna = sanitize_dna(dna);
  u32 * dp = (u32 *)&dna;
  return *dp;
}

u32 people_dna_to_model(people_dna dna, graphics_context * ctx){
  dna = sanitize_dna(dna);
  static u32_to_u32 * dna_models;
  if(dna_models == NULL) dna_models = u32_to_u32_create("nr-dna-models");
  u32 bits = people_dna_to_bit(dna);
  vec4 colors[16];
  for(u32 i = 0; i < array_count(colors); i++){
    colors[i] = vec4_new((i %2) * 0.5 + 0.5, ((i % 3) * 0.33) * 0.5 + 0.5, (i % 5) * 0.1 + 0.5, 1.0);
  }
  u32 model = 0;

  if(u32_to_u32_try_get(dna_models, &bits, &model))
    return model;
  model = index_table_alloc(ctx->models);
  model_data * md = index_table_lookup(ctx->models, model);
  u32 polygons = 1 + dna.has_circle + dna.has_stripe;
  md->polygons = index_table_alloc_sequence(ctx->polygon, polygons);
  
  polygon_data * pd = index_table_lookup_sequence(ctx->polygon, md->polygons);
  {
    index_table_resize_sequence(ctx->vertex, &(pd[0].vertexes), 4);
    vertex_data * vd = index_table_lookup_sequence(ctx->vertex, pd[0].vertexes);
    vd[0].position = vec2_new(-0.02, -0.02);
    vd[1].position = vec2_new(-0.02, 0.05);
    vd[2].position = vec2_new(0.02, -0.02);
    vd[3].position = vec2_new(0.02, 0.05);
    pd[0].material = get_unique_number();
    polygon_color_set(ctx->poly_color, pd[0].material, colors[dna.color1]);
  }
  int circoffset = 1;
  
  if(dna.has_circle){
    u32 n = 11;
    index_table_resize_sequence(ctx->vertex, &(pd[circoffset].vertexes), n);
    vertex_data * vd = index_table_lookup_sequence(ctx->vertex, pd[circoffset].vertexes);
    for(u32 i = 0; i < n; i++){
      float t = ((float) i) / ((float) n) * M_PI;
      float x = cos(t);
      float y = sin(t);
      float sign = (i % 2) * 2.0 - 1.0f;
      x = 0.02 * x;
      y = -sign * 0.02 * y;
      vd[i].position = vec2_new(x, y + 0.01 * dna.circle_offset);
    }
    pd[circoffset].material = get_unique_number();
    polygon_color_set(ctx->poly_color, pd[circoffset].material, colors[dna.color3]);
    circoffset += 1;
  }
  
  if(dna.has_stripe){
    index_table_resize_sequence(ctx->vertex, &(pd[circoffset].vertexes), 4);
    vertex_data * vd = index_table_lookup_sequence(ctx->vertex, pd[circoffset].vertexes);
    float offsety = dna.stripe_offset * 0.01f;
    vd[0].position = vec2_new(-0.02, -0.02 + offsety + 0.035);
    vd[1].position = vec2_new(-0.02, 0.00 + offsety + 0.035);
    vd[2].position = vec2_new(0.02, -0.02 + offsety + 0.035);
    vd[3].position = vec2_new(0.02, 0.00 + offsety + 0.035);
    pd[circoffset].material = get_unique_number();
    polygon_color_set(ctx->poly_color, pd[circoffset].material, colors[dna.color2]);
    circoffset += 1;
  }
  
  u32_to_u32_set(dna_models, bits, model);
  return model;
}


typedef struct{
  float time_step;
  u32 people_default_model;
}_nr_game_data;

typedef struct{
  u32 entity;
  u32 index;
}traversal_time_key;

#include "traversal_time.h"
#include "traversal_time.c"

extern node_action_table * node_actions;

// Sorts the u32s in items.
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

// Sorts the u32s in items and remove dublicates. Returns the number of distinct items.
u64 sort_distinct_u32(u32 * items, u64 cnt){
  sort_u32(items, cnt);
  u64 same = 0;
  for(u32 i = 1; i < cnt - same; i++){
    while(items[i + same] == items[i - 1])
      same++;
    items[i] = items[i + same];
  }
  return cnt - same;
}

u32_lookup * is_node_table;
u32_lookup * is_selected_table;
character_table * characters;
index_table * bezier_curve_table;
line_element * line_element_table;
connected_nodes * connected_nodes_table;

_nr_game_data * nr_game_data;
index_table * node_traversal;
u32_to_sequence * node_traversal_index;
u32_to_u32 * node_traversal_offset;
u32_to_f32 * node_traversal_timeout;
u32_lookup * visited_nodes;
nr_ui_item * ui_items;
u32_lookup * residential_nodes;
u32_lookup * city_nodes;
u32_lookup * residential_city_nodes;
traversal_time * traversal_times;

u32_to_u32 * node_sub_nodes;
u32_lookup * evil_npcs;
u32_lookup * evil_houses;

void load_connection_entity(graphics_context * ctx, u32 entity, u32 entity2){
  static connection_entities * connection_entities_table = NULL;
  if(connection_entities_table == NULL)
    connection_entities_table = connection_entities_create("connection_entities");
  if(entity > entity2)
    SWAP(entity, entity2);
  u64 connection_id = ((u64)entity) | ((u64)entity2) << 32;
  
  u32 ced = 0;
  if(connection_entities_try_get(connection_entities_table, &connection_id, &ced) == false){
    ced = index_table_alloc(ctx->entities);
    connection_entities_set(connection_entities_table, connection_id, ced);
    active_entities_set(ctx->active_entities, ced, true);
  }
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

 bool are_connected(u32 a, u32 b){
  u64 iter = 0;
  u64 index = 0;
  while(connected_nodes_iter(connected_nodes_table, &a, 1, NULL, &index, 1, &iter)){
    if(connected_nodes_table->n2[index] == b)
      return true;
  }
  return false;
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
    bool connect = true;
    if(count > 1){
      u64 indexes[count];
      connected_nodes_iter(connected_nodes_table, &entity, 1, NULL, indexes, count, NULL);
      for(u32 i = 0; i < count; i++){
	if(connected_nodes_table->n2[indexes[i]] == entity2){
	  logd("Node %i and %i are already connected!\n", entity, entity2);
	  connect = false;
	  break;
	}
      }
    }
    if(connect){
      connected_nodes_set(connected_nodes_table, entity, entity2);
      connected_nodes_set(connected_nodes_table, entity2, entity);
    }

    bool Option_Load_Entity = false;
    if(Option_Load_Entity){
      logd("Connected %i %i\n", entity2, entity);
      load_connection_entity(gctx, entity, entity2);
    }
    
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

  if(nth_str_cmp(commands, 0, "is-residential")){
    int i = 1;
    u32 node = 0;
    while(nth_parse_u32(commands, i++, &node))
      u32_lookup_set(residential_nodes, node);
    u32_lookup_print(residential_nodes);
    return true;
  }

  if(nth_str_cmp(commands, 0, "is-city")){
    int i = 1;
    u32 node = 0;
    while(nth_parse_u32(commands, i++, &node))
      u32_lookup_set(city_nodes, node);
    u32_lookup_print(city_nodes);
    return true;
  }

  if(nth_str_cmp(commands, 0, "is-active")){
    if(ctx->selection_kind != SELECTED_ENTITY || ctx->selected_index == 0){
      logd("An entity must be selected for is-active.\n");
      return true;
    }
    
    if(active_entities_get(gctx->active_entities, ctx->selected_index))
      active_entities_unset(gctx->active_entities, ctx->selected_index);
    else
      active_entities_set(gctx->active_entities, ctx->selected_index, true);
    
    return true;
  }
  
  
  if(nth_str_cmp(commands, 0, "people-default-model")){
    nth_parse_u32(commands, 1, &nr_game_data->people_default_model);
    return true;
  }
  
  if(nth_str_cmp(commands, 0, "load-people")){
    bool isevil = nth_str_cmp(commands, 1, "evil");
    u32 count = 0;
    nth_parse_u32(commands, (isevil ? 1 : 0 ) + 1, &count);
    for(u32 i = 0; i < count; i++){
      if(nr_game_data->people_default_model == 0){
	logd("Default model must be set.\n");
	goto __next;
      }

      if(residential_nodes->count == 0){
	logd("No residential nodes created.\n");
	goto __next;
      }

      if(residential_city_nodes->count != residential_nodes->count + city_nodes->count){
	u32_lookup_insert(residential_city_nodes, residential_nodes->key + 1, residential_nodes->count);
	u32_lookup_insert(residential_city_nodes, city_nodes->key + 1, city_nodes->count);
	u32_lookup_print(residential_city_nodes);
      }
      
      u32 nodeidx = randu32(residential_nodes->count);
      u32 node = residential_nodes->key[nodeidx + 1];
      
      u32 i1 = index_table_alloc(gctx->entities);
      if(isevil)
	u32_lookup_set(evil_npcs, i1);
      entity_data * ed = index_table_lookup(gctx->entities, i1);
      entity_data * node_entity = index_table_lookup(gctx->entities, node);
      ed->position = node_entity->position;
      people_dna dna = {rand(),rand(),rand(),rand(),rand(),rand(),rand()};
      ed->model = people_dna_to_model(dna, gctx);
      character_table_set(characters, i1, node, 0);
      active_entities_set(gctx->active_entities, i1, true);
      const u32 ncount = 10;
      u32 _nodes[10];
    
      _nodes[0] = node;
      _nodes[ncount - 1] = node;
      index_table_sequence seqs[10];
      static index_table * tab = NULL;
      if(tab == NULL)
	tab = index_table_create(NULL, sizeof(u32));
      index_table_clear(tab);
      u32 total_count = 1;
      for(u32 i = 1; i < ncount; i++){
	u32 idx = randu32(residential_city_nodes->count);
	u32 newnode = residential_city_nodes->key[idx + 1];
	if(i < ncount - 1)
	  _nodes[i] = newnode;
	seqs[i] = find_path(_nodes[i - 1], _nodes[i], tab, gctx);
	total_count += seqs[i].count - 1;
	if(seqs[i].count == 0){
	  logd("ERROR: Invalid path from %i to %i\n", _nodes[i-1], _nodes[i]);

	  goto __next;
	}
      }
    
      index_table_sequence seq = {0};
      u32_to_sequence_try_get(node_traversal_index, &i1, &seq);
      index_table_resize_sequence(node_traversal, &seq, total_count);

      u32 * nodes = index_table_lookup_sequence(node_traversal, seq);
      nodes[0] = node;
      traversal_time_set(traversal_times, (traversal_time_key){i1, 0}, 1.0f);
      u32 j = 1;
      for(u32 i = 1; i < ncount; i++){
	u32 * newnodes = index_table_lookup_sequence(tab, seqs[i]);
	for(u32 k = 1; k < seqs[i].count; k++){
	  nodes[j++] = newnodes[k];
	}
	traversal_time_set(traversal_times, (traversal_time_key){i1, j - 1}, 1.0f);
      }
      for(u32 i = 0; i < total_count; i++){
	if(i > 0)
	  ASSERT(nodes[i] == nodes[i -1] || are_connected(nodes[i], nodes[i - 1]));
      }
    
      u32_to_sequence_set(node_traversal_index, i1, seq);
    
    __next:;
    }
    return true;
  }
  if(nth_str_cmp(commands, 0, "find-path")){
    u32 n1, n2;
    if(nth_parse_u32(commands, 1, &n1) && nth_parse_u32(commands, 2, &n2) ){
      index_table * tab = index_table_create(NULL, sizeof(u32));
      index_table_sequence seq = find_path(n1, n2, tab, gctx);
      logd("Got path:\n");
      u32 * nodes = index_table_lookup_sequence(tab, seq);
      for(u32 i = 0; i < seq.count; i++){

	logd("%i ", nodes[i]);
      }
      logd("\n");
    }else{
      logd("Two u32s must be supplied\n");
    }
    return true;
    
  }

  if(nth_str_cmp(commands, 0, "is-sub-node")){
    u32 super, sub;
    if(nth_parse_u32(commands, 1, &super) && nth_parse_u32(commands, 2, &sub) ){
      u64 indexes[10];
      u64 index = 0;
      u64 cnt = 0;
      while((cnt = u32_to_u32_iter(node_sub_nodes, &super, 1,NULL, indexes, array_count(indexes), &index))){
	for(u32 i = 0; i < cnt; i++){
	  if(node_sub_nodes->value[indexes[i]] == sub){
	    logd("Node is already a sub-node\n");
	    return true;
	  }
	}
      }
      u32_to_u32_set(node_sub_nodes, super, sub);
    }else{
      logd("is-sub-node takes two arguments\n");
      
    }
    return true;
  }
  
  return false;
}
#include "a_star_helper.h"
#include "a_star_helper.c"
#include "f32_to_u32_lookup.h"
#include "f32_to_u32_lookup.c"
static int keycmpf32(const f32 * k1,const f32 * k2){
  if(*k1 > *k2)
    return 1;
  else if(*k1 == *k2)
    return 0;
  else return -1;
}

index_table_sequence find_path(u32 a, u32 b, index_table * outp_nodes, graphics_context * ctx){
  index_table_sequence seq ={0};
  if(a == b){
    index_table_resize_sequence(outp_nodes, &seq, 1);
    ((u32 *)index_table_lookup_sequence(outp_nodes, seq))[0] = a;
    return seq;
  }
  
  static a_star_helper * helper = NULL;
  static f32_to_u32_lookup * fronts = NULL;
  if(helper == NULL) helper = a_star_helper_create(NULL);
  if(fronts == NULL){
    fronts = f32_to_u32_lookup_create(NULL);
    fronts->cmp = (void *) keycmpf32;
  }
  f32_to_u32_lookup_clear(fronts);
  a_star_helper_clear(helper);

  entity_data * eb = index_table_lookup(ctx->entities, b);
  entity_data * ab = index_table_lookup(ctx->entities, a);
  
  f32_to_u32_lookup_set(fronts, vec3_len(vec3_sub(eb->position, ab->position)), a);
  a_star_helper_set(helper, a, vec3_len(vec3_sub(eb->position, ab->position)), 0,  (u32)-1);
  while(true){
    if(fronts->count == 0){
      // this happens if there is no path between a and b.
      return seq;
    }
    u32 node = fronts->value[1];
    entity_data * ed = index_table_lookup(ctx->entities, node);
    f32_to_u32_lookup_unset(fronts, fronts->key[1]);
    u64 helper_idx = 0;
    a_star_helper_lookup(helper, &node, &helper_idx, 1);
    ASSERT(helper_idx > 0);
    f32 traveled = helper->traveled[helper_idx];
    u64 c1 = connected_nodes_iter(connected_nodes_table, &node, 1, NULL, NULL, 1000, NULL);

    u64 ac[c1];
    connected_nodes_iter(connected_nodes_table, &node, 1, NULL, ac, c1, NULL);
    u32 cn[c1];
    for(u32 i = 0; i < c1; i++)
      cn[i] = connected_nodes_table->n2[ac[i]];
    c1 = sort_distinct_u32(cn, c1);
    
    a_star_helper_lookup(helper, cn, ac, c1);
    for(u32 i = 0; i < c1; i++){
      u32 newnode = cn[i];
      if(newnode == b){
	index_table_resize_sequence(outp_nodes, &seq, helper->count + 1);
	u32 * d = index_table_lookup_sequence(outp_nodes, seq);
	u32 cnt = 0;
	while(true){

	  d[cnt] = newnode;
	  cnt++;
	  if(node == (u32) -1){
	    for(u32 i = 0; i < cnt / 2; i++){
	      SWAP(d[i], d[cnt - i - 1]);
	    }
	    index_table_resize_sequence(outp_nodes, &seq, cnt);
	    return seq;
	  }
	  newnode = node;

	  a_star_helper_try_get(helper, &newnode, NULL, NULL,  &node);

	}
      }
      
      if(ac[i] == 0){
	entity_data * ab = index_table_lookup(ctx->entities, newnode);
	f32 d = vec3_len(vec3_sub(ab->position, ed->position)) + traveled;
	f32 expt = vec3_len(vec3_sub(ab->position, eb->position));
	f32_to_u32_lookup_set(fronts, expt + d, newnode);
	a_star_helper_set(helper, newnode, expt, d,  node);
      }
    } 
  }
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
  static u32_to_u32 * node_sub_nodes_lookup = NULL;
  static u32 last_sub_nodes_count = 0;

  if(node_sub_nodes_lookup == NULL){
    node_sub_nodes_lookup = u32_to_u32_create(NULL/*"node-sub-nodes-lookup*/);
    ((bool *)&node_sub_nodes_lookup->is_multi_table)[0] = true;
  }
  if(last_sub_nodes_count != node_sub_nodes_lookup->count || last_sub_nodes_count == 0){
    u32_to_u32_clear(node_sub_nodes_lookup);
    for(u32 i = 0; i < node_sub_nodes->count; i++){
      // construct reverse lookup.
      u32_to_u32_set(node_sub_nodes_lookup, node_sub_nodes->value[1 + i], node_sub_nodes->key[1 + i]);
    }
    last_sub_nodes_count = node_sub_nodes->count;
  }
  ctx->game_data->time_step = nr_game_data->time_step;
  u32 cnt = ctx->game_event_table->count;
  game_time += ctx->game_data->time_step * 60;
  int seconds = game_time;
  int minutes = seconds / 60;
  int hours = minutes / 60;
  UNUSED(hours);
  //logd("Game time: %02i:%02i:%02i %f\n", hours % 24, minutes % 60, seconds % 60);
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
      
      if(evt.kind == GAME_EVENT_MOUSE_BUTTON
	 && evt.mouse_button.button == mouse_button_left
	 && evt.mouse_button.pressed){
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
  static u32_lookup * current_nodes = NULL;
  if(current_nodes == NULL) current_nodes = u32_lookup_create(NULL);
  u32_lookup_clear(current_nodes);
  for(u32 i = 0; i < is_selected_table->count; i++){
    u32 entity = is_selected_table->key[i + 1];
    u32 node;
    u32 target;
    if(character_table_try_get(characters, &entity, &node, &target)){
      u32_lookup_set(current_nodes, node);
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
  for(u32 i = 0; i < characters->count; i++){
    if(u32_lookup_try_get(current_nodes, characters->current_node + i + 1)
       || u32_lookup_try_get(current_nodes, characters->target_node + i + 1)){
      u32 entity = characters->entity[1 + i];
      u32_lookup_insert(ctx->game_visible, &entity, 1);
    }
  }
  {
    u64 parents_idx[10];
    u64 index = 0;
    u32 cnt = 0;
    while((cnt = u32_to_u32_iter(node_sub_nodes_lookup, current_nodes->key + 1, 1, NULL, parents_idx, current_nodes->count, &index))){
      for(u32 i = 0; i < cnt; i++){
	u64 index = parents_idx[i];
	if(index != 0){
	  u32_lookup_set(current_nodes, node_sub_nodes_lookup->value[index]);
	}
      }
    }
  }
  for(u32 i = 0; i < visited_nodes->count; i++){
    u32 node = visited_nodes->key[1 + i];
    u32 parent = 0;
    //logd("Node: %i\n", node);
    if(u32_to_u32_try_get(node_sub_nodes_lookup, &node, &parent)){
      
      u64 indexes[10];
      u64 cnt = 0;
      u64 index = 0;
      while((cnt = u32_to_u32_iter(node_sub_nodes, &parent, 1, NULL, indexes, array_count(indexes), &index))){
	  for(u32 j = 0; j < cnt; j++){
	  //logd("Sub node: %i %i\n", node_sub_nodes->value[indexes[j]]);
	  if(node_sub_nodes->value[indexes[j]] == node ){
	    if(u32_lookup_try_get(current_nodes, &parent) || u32_lookup_try_get(current_nodes, &node)){
	      u32_lookup_insert(ctx->game_visible, &node , 1);
	      //logd("HIT\n");
	      goto exit_loop;
	    }
	  }
	}
      }
    exit_loop:;

    }else{
      u32_lookup_insert(ctx->game_visible, &node , 1);
    }
  }
  

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
  for(u32 i = 0; i < node_traversal_timeout->count; i++){
    node_traversal_timeout->value[i + 1] -= ctx->game_data->time_step;
  }
    
  for(u32 i = 0; i < characters->count; i++){
    u32 entity = characters->entity[i + 1];
    f32 timeout = 0.0;
    if(u32_to_f32_try_get(node_traversal_timeout, &entity, &timeout)){
      if(timeout <= 0)
	u32_to_f32_unset(node_traversal_timeout, entity);
      else continue;
    }
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
    vec3 targetp = ed->position;
    targetp.x += sin(entity) * 0.15;
    targetp.z += cos(entity) * 0.15;
    vec3 n2c = vec3_sub(node->position, targetp);

    auto len = vec3_len(n2c);
    if(len > 0.15){

      vec3 d = vec3_scale(n2c, 0.1 * 1.0 / len);
      entity_velocity_set(ctx->entity_velocity, entity, d);

    }else{
      entity_velocity_unset(ctx->entity_velocity, entity);
      index_table_sequence seq = {0};
      // check if there is a number of nodes in the traversal list.
      if(u32_to_sequence_try_get(node_traversal_index, &entity, &seq)){

	u32 * traversal_list = index_table_lookup_sequence(node_traversal, seq);
	
	u32 offset = 0;	
	u32_to_u32_try_get(node_traversal_offset, &entity, &offset);
	traversal_time_key tk = {entity, offset};
	f32 timeout = 0.0f;
	
	offset += 1;
	if(offset >= seq.count)
	  offset = 0;
	ASSERT(traversal_list[offset] != 0);
	characters->current_node[i + 1] = characters->target_node[i + 1];
	characters->target_node[i + 1] = traversal_list[offset];
	u32_to_u32_set(node_traversal_offset, entity, offset);
	if(traversal_time_try_get(traversal_times, &tk, &timeout)){

	  u64 index = 0;
	  u32_to_f32_lookup(node_traversal_timeout, &entity, &index, 1);
	  if(index == 0)
	    u32_to_f32_set(node_traversal_timeout, entity, timeout);
	}
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

  static u32_to_u32 * node_character_lookup = NULL;
  
  if(node_character_lookup == NULL){
    node_character_lookup = u32_to_u32_create(NULL);
    ((bool *)&node_character_lookup->is_multi_table)[0] = true;
  }
  u32_to_u32_clear(node_character_lookup);
  for(u32 i = 0; i < characters->count; i++)
    u32_to_u32_set(node_character_lookup, characters->current_node[i + 1], characters->entity[i + 1]);
  
					 
  { // do evil AI stuff
    u64 evil_nodes_idx[evil_npcs->count];
    character_table_lookup(characters, evil_npcs->key + 1, evil_nodes_idx, evil_npcs->count);
    u32 evil_nodes[evil_npcs->count];
    for(u32 i = 0; i < evil_npcs->count; i++){
      evil_nodes[i] = characters->current_node[evil_nodes_idx[i]];
      //logd("evil npcs: %i %i\n", evil_nodes[i], evil_npcs->count);
    }
    logd("\n");
    u32 cnt = sort_distinct_u32(evil_nodes, array_count(evil_nodes_idx));
    for(u32 i = 0; i < cnt; i++){
      u32 node = evil_nodes[i];
      u64 character_count = u32_to_u32_iter(node_character_lookup, &node, 1, NULL, NULL, 1000, NULL);
      u64 indexes[character_count];
      u32_to_u32_iter(node_character_lookup, &node, 1, NULL, indexes, character_count, NULL);
      u32 chars[character_count];
      for(u32 i = 0; i < character_count; i++)
	chars[i] = node_character_lookup->value[indexes[i]];
      sort_u32(chars, character_count);
      u32_lookup_lookup(evil_npcs, chars, indexes, character_count);
      u32 norm = 0, evil = 0;
      for(u32 i = 0; i < character_count; i++)
	if(indexes[i] == 0)
	  norm += 1;
	else
	  evil += 1;
      logd("Node: %i   good: %i   Evil: %i\n", node, norm, evil);
      if(norm == 1 && evil > 0){
	logd("Take over!\n");
	
      }
      
    }
    //logd("--- %i\n", cnt);
    UNUSED(cnt);
    //logd("\n");
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

  
  node_traversal_offset = u32_to_u32_create("node-traversal-offset");
  node_traversal_index = u32_to_sequence_create("node-traversal-index");
  node_traversal = index_table_create("node-traversal", sizeof(u32));
  traversal_times = traversal_time_create("node-traversal-time");
  node_traversal_timeout = u32_to_f32_create("node-traversal-timeout");
  if(ctx->game_visible == NULL){
    ctx->game_visible = u32_lookup_create("game-visible");
  }
  visited_nodes = u32_lookup_create("visited-nodes");
  residential_nodes = u32_lookup_create("residential");
  city_nodes = u32_lookup_create("city-nodes");
  residential_city_nodes = u32_lookup_create("residential-city-nodes");

  node_sub_nodes = u32_to_u32_create("node-sub-nodes");
  ((bool *)&node_sub_nodes->is_multi_table)[0] = true;
  evil_npcs = u32_lookup_create("evil npcs");;
  evil_houses = u32_lookup_create("evil houses");

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
