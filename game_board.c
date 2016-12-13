#include <iron/full.h>
#include "persist.h"
#include "sortable.h"
#include "persist_oop.h"

#include "gui.h"
#include "animation.h"

#include "game_board.h"
#include "command.h"

CREATE_TABLE_DECL(is_dead, u64, bool);

CREATE_TABLE2(body, u64, body);
//CREATE_MULTI_TABLE2(board_elements, u64, u64);

CREATE_TABLE2(target, u64, vec2);
CREATE_TABLE(is_paused, u64, bool);
CREATE_TABLE2(should_exit, u64, bool);
CREATE_TABLE(is_instant, u64, bool);
CREATE_TABLE(wielded_item, u64, u64);
CREATE_TABLE(item_command_item, u64, u64);
CREATE_TABLE2(is_wall, u64, bool);
CREATE_MULTI_TABLE(inventory, u64, u64);
CREATE_TABLE2(focused_entity, u64, u64);
CREATE_TABLE2(camera_position, u64, vec2);
CREATE_TABLE_DECL2(tile_type, u8, u64);
CREATE_TABLE2(tile_type, u8, u64);

sorttable * get_sorttable_for(const char * basename, size_t keysize, size_t value_size, u64 id){
    static struct{
    u64 * boards;
    sorttable ** tables;
    u64 cnt;
  }tables;
  for(u64 i = 0; i < tables.cnt; i++)
    if(tables.boards[i] == id)
      return tables.tables[i];
  char buf[100];
  sprintf(buf, "%s.%i", basename, id);
  sorttable tab = create_sorttable(keysize, value_size, buf);
  sorttable * newtab = iron_clone(&tab, sizeof(tab));
  list_push(tables.boards, tables.cnt, id);
  list_push2(tables.tables, tables.cnt, newtab );
  return newtab;
}


sorttable * get_sorttable(u64 game_board){
  return get_sorttable_for("game_board", sizeof(u64), sizeof(u64), game_board);
}

void add_board_element(u64 game_board, u64 target){
  sorttable * table = get_sorttable(game_board);
  sorttable_insert(table, &target, &target);
}

void remove_board_element(u64 game_board, u64 item){
  sorttable * table = get_sorttable(game_board);
  sorttable_removes(table, &item, 1);
}

size_t iter_board_elements2(u64 game_board, u64 * items, size_t item_cnt, u64 * idx){
  if(*idx == 0)
    *idx = 1;
  sorttable * table = get_sorttable(game_board);
  void * offset = table->value_area->ptr + *idx * table->value_size;
  u64 next = MIN(item_cnt, table->value_area->size / table->value_size - *idx);
  memcpy(items, offset, next * table->value_size);
  *idx += next;
  return next;
}

void measure_game_board(u64 id, vec2 * size){
  UNUSED(id);
  *size = vec2_new(0, 0);
}

void render_game_board(u64 id){
  UNUSED(id);


  rect_render(vec3_new(0.1,0.1,0.1), shared_offset, shared_size);
  {
    u64 focused_entity = get_focused_entity(id);
    if(focused_entity != 0){
      body b;
      if(try_get_body(focused_entity, &b)){
	set_camera_position(id, b.position);
      }
    }
  }
  vec2 px_size = vec2_sub(shared_size, vec2_new(0, 300)); // hack. use a vertical stackpanel to fix.

  bool do_render_floor_tiles = false;
  vec2 camera_position = get_camera_position(id);
  if(do_render_floor_tiles){
    for(int i = camera_position.x - 51; i < camera_position.x + 50; i += 1){
      for(int j = camera_position.y - 51; j < camera_position.y + 50; j += 1){
      
	u64 tile = *get_tile(i, j);
      
	if(tile == 0) continue;
	//logd("render tile..\n");
	u64 tileset = get_tileset(tile);
	vec2 position = vec2_new(i, j);
	position = vec2_sub(position, vec2_sub(camera_position, vec2_scale(px_size, 0.5 / 7.0)));
	position = vec2_scale(position, 7);
	vec2 size = vec2_new(7,7);
	vec3 color = vec3_new(1,1,1);
	animation_state anim = {.animation = tileset, .time = 0.1, .frame = 0};
	continue;
	render_animated(color, position, size, &anim);
      }
    }
  }
	
  u64 board_element_cnt = 0;
  u64 bodies[10];
  body bodyobj[array_count(bodies)];
  u64 classes[array_count(bodies)];
	      
  animation_state * animations[array_count(bodies)];
  size_t iter = 0;
  do{
    board_element_cnt = iter_board_elements2(id, bodies, array_count(bodies), &iter);
    get_refs_animation(bodies, animations, board_element_cnt);
    lookup_body(bodies,bodyobj, board_element_cnt);
    lookup_base_class(bodies, classes, board_element_cnt);
    for(u64 i = 0; i < board_element_cnt; i++){

      body b = get_body(bodies[i]);
      vec3 color;
      if(!try_get_color(bodies[i], &color)){
	color = vec3_new(1,0,0);
      }
      vec2 size = b.size;
      vec2 position = vec2_sub(b.position, vec2_sub(camera_position, vec2_scale(px_size, 0.5 / 7.0)));
      size.x = floor(size.x);
      size.y = floor(size.y);
      position.x = floor(position.x);
      position.y = floor(position.y);
      size = vec2_scale(size, 7);
      position = vec2_scale(position, 7);
      if(size.x == 0){
	size.x = 2;
	position.x -= 1;
	position.y -= 1;
	size.y += 2;
      }
      if(size.y == 0){
	size.y = 2;
	position.y -= 1;
	position.x -= 1;
	size.x += 2;
      }
      if(animations[i] != NULL){
	animations[i]->time += 0.01;	
 	render_animated(color, position, size, animations[i]);
      }else{
	rect_render(color,position , size);
      }
      set_body(bodies[i], b);
    }
    
  }while(board_element_cnt == array_count(bodies));
}

void load_game_board(u64 id){
  static bool initialized = false;
  static u64 game_board_class;
  if(!initialized){
    initialized = true;
    game_board_class = intern_string("game_board_class");
    define_method(game_board_class, render_control_method, (method)render_game_board);
    define_method(game_board_class, measure_control_method, (method)measure_game_board);
  }
  define_subclass(id, game_board_class);
}


typedef enum{
  WALL_NONE = 0,
  WALL_UP = 1,
  WALL_LEFT = 2,
  WALL_FULL = 1 | 2,
  WALL_HALF = 4
}wall_kind;

typedef struct{
  wall_kind wall_chunks[64]; //8 by 8 wall chunks
}mapchunk;

typedef struct{
  u8 floor_type[64];
}mapchunk2;
CREATE_TABLE_DECL2(map_chunks2, u64, mapchunk2);
CREATE_TABLE2(map_chunks2, u64, mapchunk2);
CREATE_TABLE2(tileset, u64, u64);
u8 * get_tile(int x, int y){
  int chunk_x = x >> 3;
  int chunk_y = y >> 3;
  x = x & 7;
  y = y & 7;
  union{
    struct{
      i16 head:1;
      i32 x:31;
      i32 y:31;
    };
   u64 index;
  }data;
  data.x = chunk_x;
  data.y = chunk_y;
  data.head = 1;
  static u64 pidx = 0;
  static mapchunk2 * chunkptr = NULL;
  if(pidx != data.index){
    pidx = data.index;
    get_refs_map_chunks2(&pidx, &chunkptr, 1);

    if(chunkptr == NULL){
      mapchunk2 m = {};
      set_map_chunks2(pidx, m);
    }
    get_refs_map_chunks2(&pidx, &chunkptr, 1);
    ASSERT(chunkptr != NULL);
  }
  return chunkptr->floor_type + x + y * 8;
}

CREATE_TABLE_DECL2(map_chunks, u64, mapchunk);
CREATE_TABLE2(map_chunks, u64, mapchunk);

CREATE_MULTI_TABLE2(visibility, u64, u64);

mapchunk * get_map_chunk_for(int x, int y){
  union{
    struct{
      i16 head:1;
      i32 x:31;
      i32 y:31;
    };
   u64 index;
  }data;
  ASSERT(sizeof(data) == 8);
  data.head = 1;
  data.x = x >> 3;
  data.y = y >> 3;
  mapchunk * ptr = NULL;
  get_refs_map_chunks(&data.index, &ptr, 1);
  if(ptr == NULL){
    mapchunk m = {};
    set_map_chunks(data.index, m);
  }
  get_refs_map_chunks(&data.index, &ptr, 1);
  ASSERT(ptr != NULL);
  return ptr;

}

wall_kind get_wall_at(int x, int y){
  mapchunk c = *get_map_chunk_for(x, y);
  x = x & 7;
  y = y & 7;
  return c.wall_chunks[x + (y << 3)];
}

void set_wall_at(int x, int y, wall_kind k){
  mapchunk * p = get_map_chunk_for(x, y);
  x = x & 7;
  y = y & 7;
  p->wall_chunks[x + (y << 3)] = k;
}

void test_walls(){
  clear_map_chunks();
  for(int i = -10; i < 10; i++){
    for(int j = -10; j < 10; j++){
      set_wall_at(i, j, (wall_kind)((i + 10) + (j + 10) * 20));
    }
  }
  for(int i = -10; i < 10; i++){
    for(int j = -10; j < 10; j++){
      int x = (int) get_wall_at(i, j);
      ASSERT(x == ((i + 10) + (j + 10) * 20));
    }
  }
  clear_map_chunks();
}

void update_game_board(u64 id){
  clear_visibility();
  void handle_commands(u64 entity){
    u64 command = 1;
    while(command != 0){
      command = get_command_queue(entity);
      if(command == 0)
	break;
      u64 cmd = get_command_invocation(command);

      command_state (* cmd2)(u64 cmd, ...) =(void *) get_method(cmd, invoke_command_method);
      bool pop = false;
      if(cmd2 == NULL){
	pop = true;
      }else{
	u64 item = get_item_command_item(command);
 	
	if(item != 0){
	  if(cmd2(command, entity, item) == CMD_DONE)
	    pop = true;
	}else{
	  if(cmd2(command, entity) == CMD_DONE)
	    pop = true;
	}
	    
      }
      if(pop)
	remove_command_queue(&entity, 1);
      else break; 
    }
   }
  handle_commands(id);
  if(get_is_paused(id))
    return;
  u64 board_element_cnt = 0;
  u64 bodies[10];
  size_t iter = 0;
  do{

    board_element_cnt = iter_board_elements2(id, bodies, array_count(bodies), &iter);
    for(u64 i = 0; i < board_element_cnt; i++){
      body body;
      if(try_get_body(bodies[i], &body)){
	bool is_wall = get_is_wall(bodies[i]);
	if(body.size.x < 0.1 && is_wall){
	  wall_kind kind = WALL_LEFT;
	  for(int y = 0; y < body.size.y; y++)
	    set_wall_at(body.position.x, y + body.position.y, kind);
	  
	}else if(body.size.y < 0.1 && is_wall){
	  wall_kind kind = WALL_UP;
	  for(int x = 0; x < body.size.x; x++)
	    set_wall_at(x + body.position.x, body.position.y, kind);
	  
	}else{
	  wall_kind kind = is_wall ? WALL_FULL : WALL_HALF;
	  for(int x = 0; x < body.size.x; x++){
	    for(int y = 0; y < body.size.y; y++)
	      set_wall_at(x + body.position.x, y + body.position.y, kind);
	  }
	}
      }
    }
  }while(board_element_cnt == array_count(bodies));
  iter = 0;
    
  do{
    board_element_cnt = iter_board_elements2(id, bodies, array_count(bodies), &iter);
    for(u64 i = 0; i < board_element_cnt; i++){
      if(get_is_dead(bodies[i])) continue;
      body b = get_body(bodies[i]);      
      u64 board_element_cnt2 = 0;
      u64 bodies2[10];
      size_t iter2 = 0;
      while((board_element_cnt2 = iter_board_elements2(id, bodies2, array_count(bodies2), &iter2))){

	for(u64 j = 0; j < board_element_cnt2; j++){
	  if(bodies2[j] == bodies[i]) continue;
	  if(get_is_dead(bodies2[j])) continue;
	  body b2 = get_body(bodies2[j]);
	  vec2 p1 = b.position;
	  vec2 p2 = b2.position;
	  vec2 dv = vec2_sub(p2, p1);
	  float l = vec2_len(dv);
	  vec2 d = vec2_scale(dv, 1/l);
	  
	  for(float v1= 0.0; v1 < l; v1+= 1){
	    vec2 tp = vec2_add(p1, vec2_scale(d, v1));
	    wall_kind wall = get_wall_at(tp.x, tp.y);
	    if(wall == WALL_UP || wall == WALL_LEFT)
	      goto next_elem;
	  }
	  set_visibility(bodies[i], bodies2[j]);
	next_elem:;
	}
      
      }
      
      handle_commands(bodies[i]);


      vec2 target;
      if(try_get_target(bodies[i], &target)){
	vec2 d = vec2_sub(target, b.position);
	float dl = vec2_len(d);
	if(dl > 0){
	  if(dl > 1.0)
	    d = vec2_scale(d, 1.0 / dl);
	  else{
	    unset_target(bodies[i]);
	  }
	  b.position = vec2_add(vec2_scale(d, 0.1), b.position);
	  bool did_collide = false;
	  for(int y = 0; y <= b.size.y; y++){
	    for(int x = 0; x <= b.size.x; x++){
	      wall_kind wall = get_wall_at(x + b.position.x, y + b.position.y);
	      if(wall == WALL_NONE) continue;
	      if(wall == WALL_FULL) {
		did_collide = true;
		goto end_col;
	      }
	      if(wall == WALL_UP && y >0){
		did_collide = true;
		goto end_col;
	      }
	      if(wall == WALL_LEFT && x > 0){
		did_collide = true;
		goto end_col;
	      }
	    }
	  }
	end_col:
	  if(!did_collide)
	    set_body(bodies[i], b);
	}
      }
    }    
  }while(board_element_cnt == array_count(bodies));
}

CREATE_MULTI_TABLE(faction_visible_items, u64, u64);
u64 get_visible_items(u64 id, u64 * items, u64 items_cnt){
  UNUSED(id);
  if(items == NULL){
    return get_faction_visible_items(1, NULL, 10000);
  }
  return get_faction_visible_items(1, items, items_cnt);
}

CREATE_STRING_TABLE(name, u64);
CREATE_TABLE2(faction, u64, u64);
