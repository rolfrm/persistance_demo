#include <iron/full.h>

bool is_whitespace(char c){
  return c == ' ' || c == '\t';
}

bool copy_nth(const char * _str, u32 index, char * buffer, u32 buffer_size){
  char * str = (char *) _str;
  while(*str != 0){
    while(is_whitespace(*str))
      str++;
    
    while(*str != 0 && is_whitespace(*str) == false){
      if(index == 0){
	*buffer = *str;
	buffer_size -= 1;
	buffer++;
      }
      str++;
    }
    if(index == 0){
      *buffer = 0;
      return true;
    }
    index--;
  }
  return false;
}

bool nth_parse_u32(char * commands, u32 idx, u32 * result){
  static char buffer[30];
  if(copy_nth(commands, idx, buffer, sizeof(buffer))){
    if(sscanf(buffer, "%i", result))
      return true;
  }
  return false;
}

bool nth_parse_f32(char * commands, u32 idx, f32 * result){
  static char buffer[30];
  if(copy_nth(commands, idx, buffer, sizeof(buffer))){
    if(sscanf(buffer, "%f", result))
      return true;
  }
  return false;
}

bool nth_str_cmp(char * commands, u32 idx, const char * comp){
  static char buffer[100];
  if(copy_nth(commands, idx, buffer, sizeof(buffer))) {
    return strcmp(buffer, comp) == 0;
  }
  return false;
}
