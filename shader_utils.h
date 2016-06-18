i32 make_shader(u32 kind, char * source, u32 length);
i32 load_simple_shader(char * vertsrc, i32 vslen, char * fragsrc, i32 fslen);
i32 load_simple_shader2(char * geomsrc, i32 gslen, char * vertsrc, i32 vslen, char * fragsrc, i32 fslen);

void uniform_vec3(int loc, vec3 value);
void uniform_mat4(int loc, mat4 mat);
