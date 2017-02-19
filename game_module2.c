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


typedef struct{
  u32 entity_id;
  union{
    struct{
      bool jmp_loosened;
      u32 found_coins;
      bool touches_ground;
    };

    u32 unused[128];
  };
}player_data;

static player_data * get_player_data(){
  static player_data * data = NULL;
  if(data == NULL){
    auto pd = create_mem_area("mod2/player");
    mem_area_realloc(pd, sizeof(player_data));
    data = pd->ptr;
  }
  return data;
}
CREATE_TABLE_DECL2(is_ground, u32, bool);
CREATE_TABLE2(is_ground, u32, bool);

CREATE_TABLE_DECL2(coin, u32, u32);
CREATE_TABLE2(coin, u32, u32);

bool game_editor_interact(graphics_context * gctx, editor_context * ctx, char * commands){
  UNUSED(gctx);UNUSED(ctx);
  auto player_data = get_player_data();
  u32 entity;
  if(nth_str_cmp(commands, 0, "set_entity") && nth_parse_u32(commands,1,&entity)){
    player_data->entity_id = entity;
    return true;
  }
  if(nth_str_cmp(commands, 0, "set_coin") && nth_parse_u32(commands,1,&entity)){
    u32 points = 1;
    nth_parse_u32(commands,2,&points);
    set_coin(entity, points);
    logd("Setting coin: %i %i\n", entity ,points);
    return true;
  }
  if(nth_str_cmp(commands, 0, "reset_coins")){
    u32 * coins = get_keys_coin();
    u64 coin_count = get_count_coin();
    for(u32 i = 0; i < coin_count; i++){
      entity_data * coin = index_table_lookup(gctx->entities, coins[i]);
      if(coin->position.y > 100){
	coin->position.y -= 1000;
      }
      vec3_print(coin->position);logd("\n");
    }
    logd("Resetting coins: %i\n", coin_count);
    return true;
  }

  if(nth_str_cmp(commands, 0, "set_ground") && nth_parse_u32(commands,1,&entity)){
    bool g;
    if(try_get_is_ground(entity, &g)){
      unset_is_ground(entity);
      logd("Unset is ground %i\n", entity);
    }else{
      set_is_ground(entity, true);
      logd("set is ground %i\n", entity);
    }
  
    return true;
  }
  
  return false;

}

static void game_update(graphics_context * ctx){
  auto player_data = get_player_data();
  //logd("Error\n");
  if(player_data->entity_id == 0){
    static bool wasHere = false;
    if(!wasHere){
      wasHere = true;
      logd("Please set entity id for the player.\n");
    }
    return;
  }
  { // camera
    entity_data * ed = index_table_lookup(ctx->entities, player_data->entity_id);
    ctx->game_data->offset.x = ed->position.x;
    ctx->game_data->offset.y = ed->position.z;
  }

  if(true){// update can jump

    static index_table * coltab = NULL;
    if(coltab == NULL)
      coltab = index_table_create(NULL, sizeof(collision_data));
    index_table_clear(coltab);
    
    u32 * gnd_entities = get_keys_is_ground();
    u32 gnd_cnt = get_count_is_ground();
    if(gnd_cnt >0){
      detect_collisions_one_way(*ctx, &(player_data->entity_id), 1, gnd_entities, gnd_cnt, coltab);
      u64 cnt2 = 0;
      collision_data * cd = index_table_all(coltab, &cnt2);
      UNUSED(cd);
      player_data->touches_ground = cnt2 != 0;
    }
  }
  
  UNUSED(ctx);
  { // handle joystick
    static bool wasJoyActive = false;
    int count;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);
    count = 0;
    const unsigned char* btn = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &count);
    if(false){
      for(int i = 0; i < count; i++){
	logd("%i ", btn[i]);
      }
      logd("\n");
    }

    bool jmp = (bool)btn[0];
    if(jmp){
      if(player_data->jmp_loosened && player_data->touches_ground)
	set_current_impulse(player_data->entity_id, vec3_new(0,0.05, 0));
      player_data->jmp_loosened = false;
    }else if(player_data->touches_ground){
      player_data->jmp_loosened = true;
    }
    
    float x = axes[0];
    float y = -axes[1];
    if(x < 0.2 && x > - 0.2)
      x = 0;
    if(y < 0.2 && y > -0.2)
      y = 0;
    UNUSED(wasJoyActive);
    if(true){
      vec2 d = vec2_new(x, y);
      float len = vec2_len(d);
      if(len < 0.1){
	d = get_entity_direction(player_data->entity_id);
	d = vec2_scale(d, 0.8);
	len = vec2_len(d);
	//vec2_print(d);logd(" %f\n", len);
	if(len < 0.01)
	  unset_entity_direction(player_data->entity_id);
	else
	  set_entity_direction(player_data->entity_id, d);
      }else
	set_entity_direction(player_data->entity_id, vec2_scale(d, 1.0));
    }
    if(x == 0 && y == 0)
      wasJoyActive = false;
    else wasJoyActive = true;
  }

  { // update coins
    static index_table * coltab = NULL;
    if(coltab == NULL)
      coltab = index_table_create(NULL, sizeof(collision_data));
    index_table_clear(coltab);
    
    u32 * coins = get_keys_coin();
    u32 coin_count = get_count_coin();
    detect_collisions_one_way(*ctx, &(player_data->entity_id), 1, coins, coin_count, coltab);
    u64 cnt2 = 0;
    collision_data * cd = index_table_all(coltab, &cnt2);
    for(u32 i = 0; i < cnt2; i++){
      player_data->found_coins += 1;

      u32 coin_id = cd[i].entity2.entity;
      logd("Coin! %i %i\n", player_data->found_coins, coin_id);
      entity_data * coin = index_table_lookup(ctx->entities, coin_id);
      coin->position.y += 1000;
    }
  }
  
}
  

void init_module(graphics_context * ctx){
  logd("INIT MODULE\n");
  graphics_context_load_update(ctx, game_update, intern_string("mod2/interactions"));
  simple_game_editor_load_func(ctx, game_editor_interact, intern_string("mod2/editor_interact"));
}
