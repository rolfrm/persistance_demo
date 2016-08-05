#include <GL/glew.h>
#include <iron/full.h>

#include "stb_image.h"

#include "persist.h"
#include "persist_oop.h"
#include "sortable.h"
#include "animation.h"
#include <GLFW/glfw3.h>
#include "game.h"
#include "gui_test.h"
#include "gui.h"

i32 load_gl_texture(const char * path){
  i32 x, y, n;
  u32 out;
  u8 * data = stbi_load(path, &x, &y, &n, 0);
  ASSERT(x > 0 && y > 0);
  
  glGenTextures(1, &out);
  glBindTexture(GL_TEXTURE_2D, out);
  if(n == 4)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x,y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x,y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  stbi_image_free(data);
  return out;
}
CREATE_TABLE(animation_texture, u64, u64);
CREATE_STRING_TABLE(textures, u64);
CREATE_TABLE(animation_textures, u64, u64);
CREATE_MULTI_TABLE(texture_sections, u64, texture_section);
CREATE_MULTI_TABLE(animation_frames, u64, animation_frame);
CREATE_TABLE(texture_info, u64, texture_info);
u64 load_pixel_frame(u64 texture, int x, int y, int x2, int y2){
  texture_info texinfo;
  if(!try_get_texture_info(texture, &texinfo)){
    char namebuf[100];
    get_textures(texture, namebuf, sizeof(namebuf));
    stbi_info(namebuf, &texinfo.width, &texinfo.height, &texinfo.comp);
    set_texture_info(texture, texinfo);
  }
  vec2 size = vec2_new(texinfo.width, texinfo.height);
  texture_section sec =
    {.uv_offset = vec2_div(vec2_new(x, y), size),
     .uv_size = vec2_div(vec2_new(x2 - x, y2 - y), size),
     .render_offset = vec2_new(0,0),
     .pixel_size = vec2_new(x2 - x, y2 - y)};
  //sec.uv_offset.y = 1.0 - sec.uv_offset.y;
  
  add_texture_sections(texture, sec);
  return 0;  
}

u64 load_pixel_frame_center_of_mass(u64 texture, int x, int y, int x2, int y2, int cx, int cy){
  texture_info texinfo;
  if(!try_get_texture_info(texture, &texinfo)){
    char namebuf[100];
    get_textures(texture, namebuf, sizeof(namebuf));
    stbi_info(namebuf, &texinfo.width, &texinfo.height, &texinfo.comp);
    set_texture_info(texture, texinfo);
  }
  vec2 size = vec2_new(texinfo.width, texinfo.height);
  int center_x = (x2 + x) / 2, center_y = (y2 + y) / 2;
  texture_section sec =
    {.uv_offset = vec2_div(vec2_new(x, y), size),
     .uv_size = vec2_div(vec2_new(x2 - x, y2 - y), size),
     .render_offset = vec2_new(center_x - cx, center_y - cy),
     .pixel_size = vec2_new(x2 - x, y2 - y)};
  
  add_texture_sections(texture, sec);
  return 0;  
}

CREATE_TABLE2(animation, u64, u64);

struct{
  u32 count;
  i32 * texture;
  u64 * texid;
}loaded_textures;

i32 get_animation_gltexture(u64 tex){

  u64 * id = memmem(loaded_textures.texid, loaded_textures.count * sizeof(u64), &tex, sizeof(u64));
  if(id != NULL){
    ASSERT(*id == tex);
    return loaded_textures.texture[id - loaded_textures.texid];
  }
  char buffer[100];
  get_textures(tex, buffer, sizeof(buffer));
  i32 newtex = load_gl_texture(buffer);
  list_push(loaded_textures.texture, loaded_textures.count, newtex);
  list_push2(loaded_textures.texid, loaded_textures.count, tex);
  return newtex;
}

void render_animated(vec3 color, vec2 offset, vec2 size, float time, u64 animation){
  UNUSED(size);
  u64 tex = get_animation_texture(animation);
  if(tex == 0) return;
  UNUSED(time);
  animation_frame frame[10];
  texture_section sec[20]; 
  size_t idx = 0;
  u64 c = iter_animation_frames(animation, frame, array_count(frame), &idx);
  u64 ts = (((double)timestamp()) * 0.000005 );
  idx = 0;
  iter_texture_sections(tex, sec, 20, &idx);
  ts = frame[ts % c].section;

  i32 gltex = get_animation_gltexture(tex);

  vec2 render_size = vec2_scale(sec[ts].pixel_size, 3);
  vec2 center = vec2_add(offset, vec2_scale(size, 0.5));
  vec2 offset2 = vec2_sub(center, vec2_scale(render_size, 0.5));
  rect_render2(color, vec2_add(offset2, sec[ts].render_offset), render_size , gltex, sec[ts].uv_offset, sec[ts].uv_size);
}
