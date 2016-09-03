#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>
//#include "gui_test.h"
#include "sortable.h"
#include "persist_oop.h"

#include "gui.h"
#include "console.h"

#include "animation.h"
#include "command.h"

#include "game_board.h"

CREATE_TABLE(damage, u64, float);
CREATE_TABLE(is_dead, u64, bool);
CREATE_TABLE(last_action, u64, u64);

vec3 vec3_rand(){
  float rnd(){ return 0.001 * (rand() % 1000); }
  return vec3_new(rnd(), rnd(), rnd());
}

void rectangle_clicked(u64 control, double x, double y){
  UNUSED(x);UNUSED(y);
  logd("Rectangle clicked!\n");
  rectangle * rect = get_rectangle(control);
  ASSERT(rect != NULL);
  if(mouse_button_action == 1)
    rect->color = vec3_new(0, 0, 0);
  else{
    rect->color = vec3_rand();
  }
}

struct {
  size_t count;
  u64_table_info * infos;
}tables;

void u64_table_initialized(u64_table_info table_info){
  UNUSED(table_info);
  logd("Initialize:.. %s\n", table_info.name);
  //list_push2(tables.infos, tables.count, table_info);
}

void erase_item(u64 id){
  for(size_t i = 0; i < tables.count; i++)
    tables.infos[i].remove(&id);
}
#include <signal.h>

void update_alien_faction(u64 alien_faction, u64 player_faction){
  static u64 move_cmd = 0;
  
  if(move_cmd == 0)
    move_cmd = intern_string("alien attack");
  u64 player = 0;
  u64 id[10], faction[10], idx = 0;
  u64 c = 0;
  while(0 != (c = iter_all_faction(id, faction, array_count(faction), &idx))){
    for(u64 i = 0; i < c; i++){
      if(faction[i] == player_faction && !get_is_dead(id[i])){
	player = id[i];
	goto player_found;
      }
    }
  }
  return;
 player_found:;
  command_arg arg = { .type = COMMAND_ARG_ENTITY, .id = player};
  u64 cmd_inv = 0;
  idx = 0;
  u64 lookup_cnt = 0;
  u64 classes[array_count(id)];
  u64 class_lookup[array_count(id)];
  //u64 indexer[array_count(id)];
  while(0 != (c = iter_all_faction(id, faction, array_count(faction), &idx))){

    for(u64 i = 0; i < c; i++){
      if(faction[i] == 0){
	//indexer[lookup_cnt] = lookup_cnt;
	class_lookup[lookup_cnt++] = id[i];
      }
    }
    lookup_base_class(class_lookup, classes, lookup_cnt);
    
    u64 j = 0;
    for(u64 i = 0; i < c; i++)
      if(faction[i] == alien_faction)
	id[j++] = id[i];
    u64 commands[j];
    memset(commands, 0, sizeof(commands));
    lookup_command_queue(id, commands, j);
    u64 k = 0;
    for(u64 i = 0; i < j; i++)
      if(commands[i] == 0)
	id[k++] = id[i];
    if(k == 0) continue;
    if(cmd_inv == 0){
      cmd_inv = get_unique_number();
      add_command_args(cmd_inv, arg);
      set_command_invocation(cmd_inv, move_cmd);
    }
    for(u64 i = 0; i < k; i++)
      commands[i] = cmd_inv;
    insert_command_queue(id, commands, k);
  }
}
CREATE_TABLE2(shooting_animation, u64, u64);
CREATE_TABLE2(idle_animation, u64, u64);
CREATE_TABLE2(walking_animation, u64, u64);
CREATE_TABLE2(is_shooting, u64, u64);
void test_gui(){
  u64 anim_tex = intern_string("Test anim");

  if(once(anim_tex)){
    set_textures(anim_tex, "anim.png");
  }
  clear_texture_sections();
  load_pixel_frame(anim_tex, 0, 10, 7, 16);
  load_pixel_frame(anim_tex, 7, 10, 14, 16);
  load_pixel_frame_center_of_mass(anim_tex, 14, 10, 21, 16, 15, 13);

  load_pixel_frame(anim_tex, 1, 17, 6, 22);
  load_pixel_frame(anim_tex, 6, 17, 11, 22);
  load_pixel_frame(anim_tex, 11, 17, 16, 22);
  load_pixel_frame(anim_tex, 16, 17, 21, 22);
  load_pixel_frame(anim_tex, 21, 17, 26, 22);
  load_pixel_frame(anim_tex, 26, 17, 31, 22);

  load_pixel_frame(anim_tex, 42, 10, 48, 16);
  load_pixel_frame_center_of_mass(anim_tex, 49, 10, 56, 16, 51 ,13);
  load_pixel_frame_center_of_mass(anim_tex, 57, 10, 64, 16, 59, 13);
  load_pixel_frame(anim_tex, 65, 10, 71, 16);

  load_pixel_frame(anim_tex, 0, 24, 9, 29);
  load_pixel_frame(anim_tex, 10, 24, 19, 29);
  
  {
    u64 anim1 = intern_string("PlayerWalk");
    if(once(anim1)){
      animation_frame frames[] = {{.section = 0, .time = 0.1},
				  {.section = 1, .time = 0.1},
				  {.section = 2, .time = 0.1}};
      u64 keys[] = {anim1, anim1, anim1};
      insert_animation_frames(keys, frames, array_count(frames));
      set_animation_texture(anim1, anim_tex);
    }
  }
  {
    u64 anim1 = intern_string("PlayerShoot");
    if(once(anim1)){
      animation_frame frames[] = {{.section = 9, .time = 0.1},
				  {.section = 10, .time = 0.1},
				  {.section = 11, .time = 0.1},
				  {.section = 12, .time = 0.1}};
      u64 keys[] = {anim1, anim1, anim1, anim1};
      insert_animation_frames(keys, frames, array_count(frames));
      set_animation_texture(anim1, anim_tex);
    }
  }
  
  u64 anim2 = intern_string("Test anim2");
  if(once(anim2)){
    animation_frame frames[] = {{.section = 3, .time = 0.1},
				{.section = 4, .time = 0.1},
				{.section = 5, .time = 0.1},
				{.section = 6, .time = 0.1},
				{.section = 7, .time = 0.1},
				{.section = 8, .time = 0.1}};
    u64 keys[] = {anim2, anim2, anim2, anim2, anim2, anim2};
    insert_animation_frames(keys, frames, array_count(frames));
    set_animation_texture(anim2, anim_tex);
  }

  u64 anim3 = intern_string("PlayerStill");
  if(once(anim3)){
    set_animation_frames(anim3, (animation_frame){.section = 0, .time = 0.1});
    set_animation_texture(anim3, anim_tex);
  }
  {
    u64 alien_anim = intern_string("alien_walk");
    if(once(alien_anim)){
      set_animation_frames(alien_anim, (animation_frame){.section = 13, .time = 0.1});
      set_animation_frames(alien_anim, (animation_frame){.section = 14, .time = 0.1});
      set_animation_texture(alien_anim, anim_tex);
    }
  }
  {
    u64 alien_anim = intern_string("alien_idle");
    if(once(alien_anim)){
      set_animation_frames(alien_anim, (animation_frame){.section = 13, .time = 10.0});
      set_animation_texture(alien_anim, anim_tex);
    }
  }
  
  init_gui();
  u64 win_id = intern_string("WIN!");
  
  make_window(win_id);
  auto m1 = get_method(win_id, render_control_method);
  ASSERT(m1 != NULL);
  window * win = get_ref_window_state(win_id);
  sprintf(win->title, "%s", "Test Window");
  logd("window opened: %p\n", win);

  u64 player_faction = intern_string("player faction");
  u64 alien_faction = intern_string("alien faction");
  
  u64 game_board = intern_string("game board");
  load_game_board(game_board);
  add_control(win_id, game_board);

  u64 ball = intern_string("Ball");
  reset_animation(ball, 0);
  if(once(ball)){
    add_faction_visible_items(1, ball);
    set_name(ball, "Ball");
    set_body(ball, (body){vec2_new(20,10), vec2_new(1,1)});
    add_board_element(game_board, ball);
  }

  u64 target = intern_string("Target");
  if(once(target)){
    add_faction_visible_items(1, target);
    set_name(target, "Target");
    set_body(target, (body){vec2_new(10, 30), vec2_new(2, 1)});
    add_board_element(game_board, target); 
  }
  u64 player;
  {
    player = intern_string("Player");
    reset_animation(player, intern_string("PlayerStill"));
    logd("Player: %i\n", player);
    set_color(player, vec3_new(0,0,1));
    set_faction(player, player_faction);
    set_focused_entity(game_board, player);
    set_shooting_animation(player, intern_string("PlayerShoot"));
    set_idle_animation(player, intern_string("PlayerStill"));
    set_walking_animation(player, intern_string("PlayerWalk"));
    if(once(player)){
      set_name(player, "Player");
      add_faction_visible_items(player_faction, player);
      set_body(player, (body){ vec2_new(1,1), vec2_new(2,2) });
      add_board_element(game_board, player);
    }
  }
  {
    u64 wall = intern_string("Pillar");
    set_is_wall(wall, true);
    set_body(wall, (body){vec2_new(14,14), vec2_new(2,2)});
    if(once(wall)){    
      add_board_element(game_board, wall);
    }
  }
  {
    u64 wall = intern_string("Pillar2");
    set_is_wall(wall, true);
    set_body(wall, (body){vec2_new(14,4), vec2_new(4,4)});
    if(once(wall)){    
      add_board_element(game_board, wall);
    }
  }
  {
    u64 wall = intern_string("Wall1");
    set_is_wall(wall, true);
    set_color(wall, vec3_new(0.2, 0.2, 0.2));
    set_body(wall, (body){vec2_new(4,4), vec2_new(10,0)});
    if(once(wall)){    
      add_board_element(game_board, wall);
    }
  }
  {
    u64 wall = intern_string("Wall2");
    set_color(wall, vec3_new(0.2, 0.2, 0.2));
    set_is_wall(wall, true);
    set_body(wall, (body){vec2_new(4,14), vec2_new(10,0)});
    if(once(wall)){    
      add_board_element(game_board, wall);
    }
  }
  {
    u64 wall = intern_string("Wall3");
    set_color(wall, vec3_new(0.2, 0.2, 0.2));
    set_is_wall(wall, true);
    set_body(wall, (body){vec2_new(4,4), vec2_new(0,10)});
    if(once(wall)){    
      add_board_element(game_board, wall);
    }
  }
  
  {
    u64 wall = intern_string("Wall4");
    set_color(wall, vec3_new(0.2, 0.2, 0.2));
    set_is_wall(wall, true);
    set_body(wall, (body){vec2_new(14,4), vec2_new(0,10)});
    if(once(wall)){    
      add_board_element(game_board, wall);
    }
  }

  u64 alien_class = intern_string("Alien Class");
  set_color(alien_class, vec3_new(1,1,1));
  set_idle_animation(alien_class, intern_string("alien_idle"));
  set_walking_animation(alien_class, intern_string("alien_idle"));
  set_faction(alien_class, alien_faction);
  
  if(true){
    for(int i = 0; i < 5/*0000*/; i++){
      u64 alienID = 0x01122000000;
      u64 alien = alienID + i;
      if(once(alien)){
	define_subclass(alien, alien_class);
	set_body(alien, (body){vec2_new(randf32() * 100, randf32() * 100), vec2_new(2,2)});
	add_board_element(game_board, alien);
      }
    }
  }
  
  {
    u64 alien = intern_string("alien2");
    if(once(alien)){
      logd("Loaded alien\n");
      add_board_element(game_board, alien);
      add_faction_visible_items(1, alien);
      define_subclass(alien, alien_class);
      set_body(alien, (body){vec2_new(50,50), vec2_new(2,2)});
      set_name(alien, "alien");
    }
  }
  
  u64 gun = intern_string("gun");
  {
    if(once(gun)){
      add_faction_visible_items(1, gun);
      set_body(gun, (body){vec2_new(50, 20), vec2_new(1, 1)});
      set_color(gun, vec3_new(0.5, 1.0, 0.5));
      set_name(gun, "gun");
      add_board_element(game_board, gun);
    }
  }
  clear_available_commands();
  command_class = intern_string("command class");
  invoke_command_method = intern_string("invoke");

  command_state invoke_command(u64 cmd, u64 id){
    logd("No handler defined for %i: %i\n", cmd, id);
    return CMD_DONE;
  }

  define_method(command_class, invoke_command_method, (method)invoke_command);
  
  u64 move_cmd = intern_string("move");
  define_subclass(move_cmd, command_class);
  add_available_commands(player, move_cmd);
  
  command_state do_move(u64 cmd, u64 id){
    command_arg arg;
    if(get_command_args(cmd, &arg, 1) == 0) return CMD_DONE;
    int x = 0, y = 0;
    if(arg.type == COMMAND_ARG_POSITION){
      x = arg.x;
      y = arg.y;
    }else{
      body b = get_body(arg.id);
      x = b.position.x;
      y = b.position.y;
    }

    body b = get_body(id);
    float d = vec2_len(vec2_sub(b.position, vec2_new(x, y)));
    set_target(id, vec2_new(x, y));
    u64 anim;
    animation_state current_anim = get_animation(id);
    
    if(d > 0.1){
      
      if(try_get_walking_animation(id, &anim) && anim != current_anim.animation)
	reset_animation(id, anim);
      return CMD_NOT_DONE;
    }else{
      if(try_get_idle_animation(id, &anim))
	reset_animation(id, anim);
      return CMD_DONE;
    }
  }
  define_method(move_cmd, invoke_command_method, (method) do_move);

  command_state do_stop(u64 cmd, u64 id){
    UNUSED(cmd);
    u64 clear[] = {id, id ,id, id, id, id, id, id};
    remove_command_queue(clear, array_count(clear));
    body b = get_body(id);
    set_target(id, b.position);
    set_body(id, b);
    u64 idle = 0;
    if(try_get_idle_animation(id, &idle))
      reset_animation(id, idle);
    return CMD_DONE;
  }
  
  u64 stop_cmd = intern_string("stop");
  define_subclass(stop_cmd, command_class);
  add_available_commands(player, stop_cmd);
  define_method(stop_cmd, invoke_command_method, (method) do_stop);
  set_is_instant(stop_cmd, true);

  command_state do_take(u64 cmd, u64 id){
    while(do_move(cmd, id) == CMD_NOT_DONE)
      return CMD_NOT_DONE;
    command_arg arg;
    if(get_command_args(cmd, &arg, 1) == 0) return CMD_DONE;
    if(arg.type == COMMAND_ARG_ENTITY){
      add_inventory(id, arg.id);
      logd("ADDED to inventory\n");
      remove_board_element(game_board, arg.id);
    }
    return CMD_DONE;
  }
  
  u64 take_cmd = intern_string("take");
  define_subclass(take_cmd, command_class);
  add_available_commands(player, take_cmd);
  define_method(take_cmd, invoke_command_method, (method) do_take);

  command_state do_drop(u64 cmd, u64 id){
    UNUSED(cmd);UNUSED(id);
    command_arg arg;
    if(get_command_args(cmd, &arg, 1) == 0) return CMD_DONE;
    if(arg.type == COMMAND_ARG_ITEM){
      body b = get_body(arg.id);
      b.position = get_body(id).position;
      set_body(arg.id, b);
      add_board_element(game_board, arg.id);

      u64 item, it = 0;
      int c = 0;
      while((c = iter_inventory(id, &item, 1,  &it)) > 0){
	if(item == arg.id)
	  clear_at_inventory(it - 1);
      }
    }
    return CMD_DONE;
  }
  
  u64 drop_cmd = intern_string("drop");
  define_subclass(drop_cmd, command_class);
  add_available_commands(player, drop_cmd);
  define_method(drop_cmd, invoke_command_method, (method) do_drop);
  
  u64 pause_cmd = intern_string("pause");
  command_state _pause_board(u64 cmd, u64 id){
    UNUSED(cmd);
    bool newstate = !get_is_paused(id);
    if(newstate){
      logd("Pause!\n");
    }else{
      logd("Unpause!\n");
    }
    set_is_paused(id, newstate );
    return CMD_DONE;
  }
  define_method(pause_cmd, invoke_command_method, (method)_pause_board);
  define_subclass(pause_cmd, command_class);
  add_available_commands(game_board, pause_cmd);

  u64 wield_cmd = intern_string("wield");
  command_state wield(u64 cmd, u64 id){
    command_arg arg;
    if(get_command_args(cmd, &arg, 1) == 0) return CMD_DONE;
    if(arg.type == COMMAND_ARG_ITEM){
      set_wielded_item(id, arg.id);
      char namebuf[100];
      get_name(get_wielded_item(id), namebuf, sizeof(namebuf));
      logd("Wielding: %s %i %i \n", namebuf, arg.id, get_wielded_item(id));
    }
    return CMD_DONE;
  }
  define_subclass(wield_cmd, command_class);
  define_method(wield_cmd, invoke_command_method, (method) wield);
  add_available_commands(player, wield_cmd);
  
  command_state exit_board(u64 cmd, u64 id){
    UNUSED(cmd);
    logd("Exiting\n");
    set_should_exit(id, true);
    return CMD_DONE;
  }
  u64 exit_cmd = intern_string("exit");
  define_subclass(exit_cmd, command_class);
  add_available_commands(game_board, exit_cmd);
  define_method(exit_cmd, invoke_command_method, (method) exit_board);
  set_should_exit(game_board, false);


  command_state do_erase(u64 cmd, u64 id){
    UNUSED(id);
    command_arg arg;
    if(get_command_args(cmd, &arg, 1) == 0) return CMD_DONE;
    if(arg.type == COMMAND_ARG_ENTITY){
      erase_item(arg.id);
    }
    return CMD_DONE;
  }
  u64 erase_cmd = intern_string("erase");
  define_subclass(erase_cmd, command_class);
  add_available_commands(game_board, erase_cmd);
  define_method(erase_cmd, invoke_command_method, (method) do_erase);
  
  command_state do_throw(u64 cmd, u64 id, u64 itemid){
    command_arg arg;
    logd("Executing throw!\n");
    if(get_command_args(cmd, &arg, 1) == 0) return CMD_DONE;
    if(arg.type == COMMAND_ARG_ENTITY){
      logd("Throwing %i from %i to hit %i\n", id, itemid, arg.id);
      add_board_element(game_board, itemid);
      u64 item, it = 0;
      int c = 0;
      while((c = iter_inventory(id, &item, 1,  &it)) > 0){
	if(item == itemid)
	  clear_at_inventory(it - 1);
      }
      body b1 = get_body(itemid);
      b1.position = get_body(id).position;
      set_body(itemid, b1);
      body b = get_body(arg.id);
      set_target(itemid, b.position);
    }
    return CMD_DONE;
  }
  
  u64 throw_cmd = intern_string("throw");
  define_subclass(throw_cmd, command_class);
  define_method(throw_cmd, invoke_command_method, (method) do_throw);
  add_available_commands(ball, throw_cmd);
  add_available_commands(gun, throw_cmd);
  command_state do_ls(u64 cmd, u64 id){
    UNUSED(cmd);
    logd("List cmd..\n");
    u64 item[1], it = 0;
    u64 i = 0;
    u64 c;
    u64 inventory_size = get_inventory(id, NULL, 10000);
    logd("Listing inventory of %i items", inventory_size);
    while((c = iter_inventory(id, item, array_count(item),  &it)) > 0){
      char name[100];
      get_name(item[0], name, sizeof(name));
      logd("%i : %s", i++, name);
    }
    return CMD_DONE;
  }
  
  u64 ls_cmd = intern_string("ls");
  define_subclass(ls_cmd, command_class);
  define_method(ls_cmd, invoke_command_method, (method) do_ls);
  add_available_commands(player, ls_cmd);

  command_state do_look(u64 cmd, u64 id){
    UNUSED(cmd);
    u64 idx = 0;
    u64 items[10];
    u64 cnt;
    bool any = false;
    //
    while((cnt = iter2_visibility(id, items, array_count(items), &idx)) > 0){
      logd("Cnt: %i %i %i\n", cnt, id, items[0]);

      if(!any){
	logd("I see: \n");
	any = true;
      }
      
      for(u64 j = 0; j < cnt; j++){
	char name[100];
	u64 l = get_name(items[j], name, sizeof(name));
	if(l == 0){
	  logd(" - Unknown object (%i)\n", items[j]);
	}else{
	  logd(" - '%s'\n", name);
	}
      }
    }
    if(!any){
      logd("I see nothing.\n");
    }
    
    return CMD_DONE;
  }
  
  u64 look_cmd = intern_string("look");
  define_subclass(look_cmd, command_class);
  define_method(look_cmd, invoke_command_method, (method) do_look);
  add_available_commands(player, look_cmd);
  set_is_instant(look_cmd, true);
  command_state do_alien_attack(u64 cmd, u64 id){
    command_arg arg;
    if(get_command_args(cmd, &arg, 1) == 0 || arg.type != COMMAND_ARG_ENTITY)
      return CMD_DONE;
    if(get_is_dead(arg.id)){
      remove_target(&id, 1);
      return CMD_DONE;
    }
    
    if(CMD_NOT_DONE == do_move(cmd, id)) return CMD_NOT_DONE;
    
    u64 lst = get_last_action(id);
    u64 cooldown = 500000; //500 us.
    if(timestamp() - lst > cooldown){
      if(randf32() < 0.3){
	float dmg = floor(randf32() * 4.0 + 1.0);
	set_damage(arg.id, get_damage(arg.id) + dmg);
      }
      set_last_action(id, timestamp());
    }
    
    return CMD_NOT_DONE;
  }
  
  u64 alien_attack_cmd = intern_string("alien attack");
  define_subclass(alien_attack_cmd, command_class);
  define_method(alien_attack_cmd, invoke_command_method, (method) do_alien_attack);

  command_state do_shoot(u64 cmd, u64 id, u64 itemid){
    u64 is_shooting_ts;
    
    if(try_get_is_shooting(id, &is_shooting_ts)){
      if(is_shooting_ts > timestamp())
	return CMD_NOT_DONE;
      u64 idle;
      if(try_get_idle_animation(id, &idle))
	reset_animation(id, idle);
      remove_is_shooting(&id, 1);
      return CMD_DONE;
    }
    
    static u64 bulletId = 0x11122000000;
    command_arg arg;
    if(get_command_args(cmd, &arg, 1) == 0) return CMD_DONE;
    if(arg.type == COMMAND_ARG_ITEM) return CMD_DONE;
    u64 ts = get_last_action(itemid);
    u64 ts1 = timestamp();
    u64 cooldown = 500000;

    if(ts1 - ts < cooldown)
      return CMD_NOT_DONE;
    
    body shooter_body = get_body(id);
    body target_body = get_body(arg.id);
    u64 bullet = bulletId++;
    set_body(bullet, (body){shooter_body.position, vec2_new(1,1)});
    add_board_element(game_board, bullet);
    set_target(bullet, target_body.position);
    set_color(bullet, vec3_new(1,1,1));
    set_last_action(itemid, ts1);
    u64 shooting_animation;
    if(try_get_shooting_animation(id, &shooting_animation)){
      u64 n_frames = 20;
      animation_frame frames[n_frames];
      memset(frames, 0, sizeof(frames));
      u64 keys[n_frames];
      for(u64 i = 0; i < n_frames; i++)
	keys[i] = shooting_animation;
      lookup_animation_frames(keys, frames, n_frames);
      
      float total_time = 0.0;
      for(u64 i = 0; i < n_frames && frames[i].time > 0.0f; i++)
	total_time += frames[i].time;
      set_is_shooting(id, ts1 + total_time * 1e6);
      reset_animation(id, shooting_animation);
      return CMD_NOT_DONE;
    }else
      return CMD_DONE;
  }

  u64 shoot_cmd = intern_string("shoot");
  define_subclass(shoot_cmd, command_class);
  define_method(shoot_cmd, invoke_command_method, (method) do_shoot);
  add_available_commands(gun, shoot_cmd);  
  
  u64 console = intern_string("console");
  set_focused_element(win_id, console);
  set_console_height(console, 300);
  create_console(console);
  add_control(win_id, console);
  set_margin(console, (thickness){5,5,5,5});
  set_vertical_alignment(console, VALIGN_BOTTOM);
  set_console_history_cnt(console, 100);

  void command_entered(u64 id, char * cmd){
    UNUSED(id);
    bool parse_cmd(u64 id, u64 wielder){
      command_arg args[10];
      u64 arg_cnt = 10;
      u64 found_cmd = parse_command(id, cmd, args, &arg_cnt);
      if(found_cmd != 0){
	u64 cmd_inv = get_unique_number();
	set_command_invocation(cmd_inv, found_cmd);
	for(u64 i = 0; i < arg_cnt; i++){
	  add_command_args(cmd_inv, args[i]);
	}
	if(wielder != 0){
	  if(get_is_instant(found_cmd)){
	    command_state (* cmd2)(u64 id, u64 id2, u64 id3) =(void *) get_method(found_cmd, invoke_command_method);
	    if(cmd2 != NULL)
	      cmd2(cmd_inv, wielder, id);
	    
	  }else{
	    logd("Wielder command!\n");
	    set_item_command_item(cmd_inv, id);
	    insert_command_queue(&wielder, &cmd_inv, 1);
	  }
	}else{
	
	  if(get_is_instant(found_cmd)){
	    command_state (* cmd2)(u64 id, u64 id2) =(void *) get_method(found_cmd, invoke_command_method);
	    if(cmd2 != NULL)
	      cmd2(cmd_inv, id);
	    
	  }else
	    insert_command_queue(&id, &cmd_inv, 1);
	}
	return true;
      }
      return false;
    }
    if(parse_cmd(player, 0) || parse_cmd(game_board, 0)){
    }else{
      u64 wielded_item = get_wielded_item(player);
      if(wielded_item != 0 && parse_cmd(wielded_item, player)){
      }else{
	logd("Error: Unknown command '%s'\n", cmd);
      }
    }
  }

  define_method(console, console_command_entered_method, (method)command_entered);
  
  void check_pause(u64 console, int key, int mods, int action){
    if(key == key_space && mods == mod_ctrl && action == key_press){
      _pause_board(0, game_board);
    }else{
      CALL_BASE_METHOD(console, key_handler_method, key, mods, action);
    }
  }
  
  define_method(console, key_handler_method, (method)check_pause);

  auto prev_printer = iron_log_printer;
  
  //
  void print_console(const char * fmt, va_list lst){
    int l = strlen(fmt);
    if(l == 0) return;
    char buf[200];
    va_list ap;
    va_copy(ap, lst);
    vsprintf(buf, fmt, lst);
    push_console_history(console, buf);
    prev_printer(fmt, ap);
  }
  
  iron_log_printer = print_console;
  
  logd("Player: %i\n", player);
  
  bool should_exit = false;
  while(!(should_exit = get_should_exit(game_board))){
    u64 time_start = timestamp();
    if(!get_is_paused(game_board)){

      u64 bodies[10];
      u64 board_element_cnt = 0;
      size_t iter = 0;
      do{
	board_element_cnt = iter_board_elements2(game_board, bodies, array_count(bodies), &iter);
	for(u64 i = 0; i < board_element_cnt; i++){
	  u64 id = bodies[i];
	  animation_state * get_animation;
	  get_refs_animation(&id, &get_animation, 1);
	  if(get_animation == NULL){
	    u64 anim = get_idle_animation(id);
	    if(anim == 0){
	      u64 bid = 0;
	      u64 base = 0;
	      while ((0 !=  (base = get_baseclass(id, &bid))) && anim == 0)
		anim = get_idle_animation(base);
	    }
	    if(anim != 0)
	      reset_animation(id, anim);
	  }
	}
      }while(board_element_cnt == array_count(bodies));

      u64 id[10], idx = 0;
      float dmg[10];
      u64 c;
      while(0 != (c = iter_all_damage(id, dmg, array_count(dmg), &idx))){
	for(u64 i = 0; i < c; i++){
	  logd("DEAD? %f\n", dmg[i]);
	  if(dmg[i] > 20){
	    logd("DEAD! %f\n", dmg[i]);
	    set_is_dead(id[i], true);
	    unset_damage(id[i]);
	  }
	}
      }
      update_alien_faction(alien_faction, player_faction);
    }
    update_game_board(game_board);
    u64 time_start2 = timestamp();
    u64 dt = time_start2 - time_start;
    UNUSED(dt);
    //logd("DT: %i\n", time_start2 - time_start);
    window w;
    u64 id, j=0;
    bool any_active = false;    
    while(1 == iter_all_window_state(&id, &w, 1, &j)){
      if(id == 0) continue;
      any_active = true;
      auto method = get_method(id, render_control_method);
      ASSERT(method != NULL);
      if(method!= NULL) method(id);      
    }
    
    if(!any_active){
      logd("Ending program\n");
      break;
    }
    glfwPollEvents();
    iron_sleep(0.01);
  }
}
