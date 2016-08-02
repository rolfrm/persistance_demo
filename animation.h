i32 load_gl_texture(const char * path);

typedef struct{
  int width, height;
  int comp;
}texture_info;

typedef struct{
  vec2 uv_offset;
  vec2 uv_size;
  vec2 render_offset;
  vec2 pixel_size;
}texture_section;

typedef struct{
  u64 section;
  float time;
}animation_frame;

CREATE_STRING_TABLE_DECL(textures, u64);
CREATE_TABLE_DECL(animation_texture, u64, u64);
CREATE_MULTI_TABLE_DECL(texture_sections, u64, texture_section);
CREATE_MULTI_TABLE_DECL(animation_frames, u64, animation_frame);
CREATE_TABLE_DECL(texture_info, u64, texture_info);
CREATE_TABLE_DECL(animation, u64, u64);
u64 load_pixel_frame(u64 texture, int x, int y, int x2, int y2);
u64 load_pixel_frame_center_of_mass(u64 texture, int x, int y, int x2, int y2, int cx, int cy);
//u64 render_animated(vec2 offset, vec2 size, float time, u64 animation);
void render_animated(vec3 color, vec2 offset, vec2 size, float time, u64 animation);
