#include <iron/full.h>
#include "persist.h"
#include "sortable.h"
#include "persist_oop.h"
#include "gui.h"

#include "command.h"

#include "game_board.h"

CREATE_TABLE(command_invocation, u64, u64);
CREATE_MULTI_TABLE2(command_queue, u64, u64);
CREATE_MULTI_TABLE(command_args, u64, command_arg);
CREATE_MULTI_TABLE(available_commands, u64, u64);
CREATE_STRING_TABLE(command_name, u64);

u64 invoke_command_method = 0;
u64 command_class = 0;

u64 parse_command(u64 id, const char * str, command_arg * out_args, u64 * in_out_cnt){
  u64 visible_item_cnt = get_visible_items(id, NULL, 0);
  u64 visible_items[visible_item_cnt];
  get_visible_items(id, visible_items, visible_item_cnt);

  u64 inventory_cnt = get_inventory(id, NULL, 10000);
  u64 inventory[inventory_cnt];
  get_inventory(id, inventory, inventory_cnt);
  
  *in_out_cnt = 0;
  u64 avail_commands[32];
  u64 idx = 0;
  u64 cnt = 0;
  while(0 < (cnt = iter_available_commands(id, avail_commands, array_count(avail_commands), &idx))){
    for(u64 i = 0; i < cnt; i++){
      named_item nm = unintern_string(avail_commands[i]);
      if(false && starts_with(str, nm.name)){

      }else{
	char buf[33];
	int offset = 0;
	if(strcmp(str,nm.name) == 0){
	  offset = sprintf(buf, "%s", nm.name);
	}else{
	  offset = sprintf(buf, "%s ", nm.name);
	}
	
	if(starts_with(buf, str)){
	  char * args_start = ((char *)str) + offset;
	  while(*args_start != 0){
	    while(*args_start == ' ')
	      args_start += 1;
	    if(*args_start == 0) break;
	    int x, y;
	    int c = sscanf(args_start, "%i ", &x);

	    if(c == 1){
	      while(*args_start != ' ' && *args_start != 0)
		args_start += 1;
	      
	      while(*args_start == ' ')
		args_start += 1;
	      c += sscanf(args_start, "%i", &y);
	      while(*args_start != ' ' && *args_start != 0)
		args_start += 1;
	    }
	    if(c == 2){ 
	      out_args->type = COMMAND_ARG_POSITION;
	      out_args->x = x;
	      out_args->y = y;
	      *in_out_cnt += 1;
	      out_args += 1;
	      continue;
	    }else if(c == 1)
	      return 0;
	    
	    c = sscanf(args_start, "%s ", buf );
	    
	    if(c == 1 && strlen(buf) > 0){
	      u64 prevcnt = *in_out_cnt;
	      for(u64 i = 0; i < visible_item_cnt; i++){
		char buf2[33];
		u64 c2 = get_name(visible_items[i], buf2, 33);
	        if(c2 > 0 && strcmp(buf2, buf) == 0){
		  
		  args_start += strlen(buf2);
		  out_args->type = COMMAND_ARG_ENTITY;
		  out_args->id = visible_items[i];
		  *in_out_cnt += 1;
		  break;
		}
	      }
	      for(u64 i = 0; i < inventory_cnt; i++){
		char buf2[33];
		u64 c2 = get_name(inventory[i], buf2, 33);
	        if(c2 > 0 && strcmp(buf2, buf) == 0){
		  
		  args_start += strlen(buf2);
		  out_args->type = COMMAND_ARG_ITEM;
		  out_args->id = inventory[i];
		  *in_out_cnt += 1;
		  break;
		}
	      }
	      if(prevcnt == *in_out_cnt){
		return 0;
	      }
	    }else return 0;
	    
	  }
	  
	  return avail_commands[i];
	}
      }
    }
  }
  return 0;
}
