#include <iron/full.h>
#include "persist.h"
#include "sortable.h"
#include "persist_oop.h"
#include "gui.h"
#include "index_table.h"
#include "simple_graphics.h"
#include "abstract_sortable.h"
#include "u32_lookup.h"
#include "u32_to_u32.h"
#include "node_roguelike.h"
#include "node_action_table.h"
#include "node_action_table.c"

u32_to_u32 * ui_subnodes;
u32_to_u32 * ui_node_parent;
node_action_table * node_actions;
u32_to_u32 * ui_node_actions;
u32_to_u32 * node_to_entity;
static u32_to_u32 * ui_node_index;
u32_lookup * shown_ui_nodes;
void node_roguelike_ui_init(){
  ui_subnodes = u32_to_u32_create("ui_subnodes");
  ui_node_parent = u32_to_u32_create("ui_node_parent");
  node_actions = node_action_table_create(NULL);
  ui_node_actions = u32_to_u32_create("ui_node_actions");
  ((bool *)&ui_subnodes->is_multi_table)[0] = true;
  node_to_entity = u32_to_u32_create("ui_node_entities");
  shown_ui_nodes = u32_lookup_create("ui_node_shown");
  ui_node_index = u32_to_u32_create("ui_node_index");
}

void node_roguelike_ui_update(graphics_context * ctx){

  u64 indexes[shown_ui_nodes->count];
  memset(indexes, 0, sizeof(indexes));
  u32_to_u32_lookup(node_to_entity, shown_ui_nodes->key + 1,indexes, array_count(indexes));
  {
    // update node->parent
    static u32 lastcount = 0;
    if(ui_subnodes->count != lastcount){
      u32 parent = 0;
      u32 index = 0;
      for(u32 i = 0; i < ui_subnodes->count; i++){

	u32 thisparent = ui_subnodes->key[i + 1];
	if(thisparent != parent){
	  parent = thisparent;
	  index = 0;
	}

	u32_to_u32_insert(ui_node_parent, ui_subnodes->value + i + 1, &parent, 1);
	if(index > 0)
	  u32_to_u32_insert(ui_node_index, ui_subnodes->value + i + 1, &index, 1);
	index++;
      }
      lastcount = ui_subnodes->count;
    }
  }

  { // insert new entities.
    u32 zeros = 0;
    for(u32 i = 0; i < array_count(indexes); i++){
      if(indexes[i] == 0){
	zeros++;
      }
    }
    
    u32 new_entries_nodes[zeros];
    u32 new_entries_entities[zeros];
    u32 j = 0;
    for(u32 i = 0; i < array_count(indexes); i++){
      if(indexes[i] == 0){
	
	u32 i1 = index_table_alloc(ctx->entities);
	new_entries_nodes[j] = shown_ui_nodes->key[i + 1];
	new_entries_entities[j] = i1;
	j++;
      }
    }
    u32_to_u32_insert(node_to_entity, new_entries_nodes, new_entries_entities, zeros);
  }

  u32_to_u32_lookup(node_to_entity, shown_ui_nodes->key + 1,indexes, array_count(indexes));
  for(u32 i = 0; i < array_count(indexes); i++){
    bool isActive = false;
    if(!active_entities_try_get(ctx->active_entities, node_to_entity->value[indexes[i]], &isActive) || !isActive){
      active_entities_set(ctx->active_entities, node_to_entity->value[indexes[i]], true);
    }
  }

  for(u32 i = 0; i < ui_node_parent->count; i++){
    u32 parent = 0;
    u32 node = shown_ui_nodes->key[i + 1];
    if(u32_to_u32_try_get(ui_node_parent, &node, &parent)){
      u32 entity;
      u32 parent_entity;
      if(u32_to_u32_try_get(node_to_entity, &parent, &parent_entity)
	 && u32_to_u32_try_get(node_to_entity, &node, &entity)){
	u32 index = 0;
	u32_to_u32_try_get(ui_node_index, &node, &index);
	entity_data * ed = index_table_lookup(ctx->entities, entity);
	entity_data * ped = index_table_lookup(ctx->entities, parent_entity);

	ed->position = vec3_add(ped->position, vec3_new(0.1, -0.1 * index, 0));
	
      }
    }
  }
  
  
}

void node_roguelike_ui_register(u32 actionid, ui_node_action action){
  node_action_table_set(node_actions, actionid, action);
}

void node_roguelike_ui_show(u32 uinode, u32 entity){

  u64 cnt = u32_to_u32_iter(ui_subnodes, &uinode, 1, NULL, NULL, 10000, NULL);
  logd("loading: %i sub nodes:%i\n", uinode, cnt);
  u32 nodes[cnt];
  {
    u64 indexes[cnt];

    u32_to_u32_iter(ui_subnodes, &uinode, 1, NULL, indexes, cnt, NULL);
    for(u32 i = 0; i < cnt; i++)
      nodes[i] = ui_subnodes->value[indexes[i]];
  }
  
  sort_u32(nodes, cnt);
  
  //u32_lookup_clear(shown_ui_nodes);
  u32_lookup_insert(shown_ui_nodes, nodes, cnt);
  
  if(entity != 0)
    u32_to_u32_insert(node_to_entity, &uinode, &entity, 1);
  for(u32 i = 0; i < cnt; i++){
    
    node_roguelike_ui_show(nodes[i], 0);
  }
}
