
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
void table2_test();
void test_walls();
void test_persist_oop();
bool test_elf_reader2();
bool test_elf_reader();
bool test_type_system();
void test_simple_graphics();

u32 index_table_capacity(index_table * table){
  return table->ptr->size / table->element_size;
}

u32 index_table_count(index_table * table){
  return ((u32 *) table->ptr->ptr)[0];
}

void index_table_count_set(index_table * table, u32 newcount){
  ((u32 *) table->ptr->ptr)[0] = newcount;
}

u32 _index_table_free_index_count(index_table * table){
  return ((u32 *) table->free_indexes->ptr)[0];
}

u32 alloc_index_table(index_table * table){
  u32 freeindexcnt = _index_table_free_index_count(table);
  if(freeindexcnt > 0){
    u32 idx = ((u32 *) table->free_indexes->ptr)[freeindexcnt - 1];
    mem_area_realloc(table->free_indexes, table->free_indexes->size - table->element_size);
    return idx;
  }
  while(index_table_capacity(table) <= index_table_count(table)){
    u32 prevsize = table->ptr->size;
    mem_area_realloc(table->ptr, MAX(table->ptr->size * 2, 8 * table->element_size));
    memset(table->ptr + prevsize, 0,  table->ptr->size - prevsize);
  }
  u32 idx = index_table_count(table);
  index_table_count_set(table,idx + 1);
  return idx;
}

void index_table_remove(index_table * table, u32 index){
  u32 cnt = _index_table_free_index_count(table);
  mem_area_realloc(table->free_indexes, table->free_indexes->size + table->element_size);
  ((u32 *)table->free_indexes->ptr)[cnt] = index;
  ((u32 *) table->free_indexes->ptr)[0] += 1;
}

index_table * create_index_table(const char * name, u32 element_size){
  index_table table = {0};
  table.element_size = element_size;

  if(name == NULL){
    table.ptr = mem_area_create(name);
    char name2[128];
    sprintf(name2, "%s.free", name);
    table.free_indexes = mem_area_create(name2);
  }else{
    table.ptr = mem_area_create_non_persisted();
    table.free_indexes = mem_area_create_non_persisted();
  }
  
  if(table.free_indexes->size < sizeof(u32)){
    mem_area_realloc(table.free_indexes, sizeof(u32));
    memset(table.free_indexes->ptr, 0, sizeof(u32));
  }

  if(table.ptr->size < element_size){
    while(table.ptr->size < sizeof(u32))
      mem_area_realloc(table.ptr, table.ptr->size + element_size );
    index_table_count_set(&table, 0);
  }
  
  return iron_clone(&table, sizeof(table));
}

void * lookup_index_table(index_table * table, u32 index){
  ASSERT(index < index_table_count(table));
  return table->ptr + index * table->element_size;
}

u32 iterate_voxel_chunk(index_table * table, u32 start_index){
  u32 count = 0;
  u32 * ref = lookup_index_table(table, start_index);
  for(u32 i = 0; i < 8; i++){
    u32 data = ref[i];
    if(data == 0) continue;
    if(data > index_table_count(table)){
      u32 color = ~data;
      logd("#%p ", color);
      count++;
    }else{
      count += iterate_voxel_chunk(table, data);
    }
  }
  return count;
}
CREATE_TABLE_DECL2(camera_3d_position, u64, mat4)
CREATE_TABLE2(camera_3d_position, u64, mat4);

typedef struct{
  index_table * voxels;
  index_table * materials;
  u32 index;
}voxel_board_data;
CREATE_TABLE_DECL2(board_data2, u64, voxel_board_data);
CREATE_TABLE2(board_data2, u64, voxel_board_data);


vec4 u32_to_color(u32 colorid){
  vec4 col_255 = vec4_new((colorid >> 24) & 0xFF, (colorid >> 16) & 0xFF, (colorid >> 8) & 0xFF, colorid & 0xFF);
  return vec4_scale(col_255, 1.0 / 255.0);
}

typedef enum{
  VOX_X_SURFACE,
  VOX_Y_SURFACE,
  VOX_Z_SURFACE
}vox_surface;
typedef struct{
  int x;
  int y;
  int z;
  u8 lod;
  vox_surface type;
}voxel_collider_key;

CREATE_TABLE_DECL2(voxel_collider, voxel_collider_key, u32);
CREATE_TABLE2(voxel_collider, voxel_collider_key, u32);


int voxel_collider_key_cmp(const voxel_collider_key * a, const voxel_collider_key * b){
  return memcmp(a, b, sizeof(voxel_collider_key));
}

void calc_voxel_surface(index_table * vox, u32 idx){
  static bool was_initialized = false;
  sorttable * tab = voxel_collider_initialize();
  if(!was_initialized){
    
    tab->cmp = (void*)voxel_collider_key_cmp;
    was_initialized = true;
  }
  clear_voxel_collider();
  UNUSED(vox);UNUSED(idx);

  void insert_surface(u32 idx2, int x, int y, int z, int lod){
    
    if(idx2 >= index_table_count(vox)){
      voxel_collider_key key = {.x = x, .y = y, .z = z, .lod = lod, .type = VOX_X_SURFACE};
      vox_surface a[] = {VOX_X_SURFACE, VOX_Y_SURFACE, VOX_Z_SURFACE};
      int * pts[] = {&key.x, &key.y, &key.z};
      int x[] = {0, 1};
      
      for(u32 i = 0; i < array_count(a); i++){
	for(u32 j = 0; j < array_count(x); j++){
	  for(u32 k = 0; k < array_count(pts); k++){
	    key.type = a[i];
	    *pts[k] += x[j];
	    u32 value = 0;
	    logd("%i %i %i %i \n", key.x, key.y, key.z, key.lod);
	    if(!try_get_voxel_collider(key, &value) || value == 0)
	      insert_voxel_collider(&key, &idx2, 1);
	    *pts[k] -= x[j];
	    if(value != 0)
	      logd("Collision!");
	  }
	}
      }
      logd("\n");
    }else{
      u32 * k = lookup_index_table(vox, idx2);
      for(u32 i = 0; i < 8; i++){
	if(k[i] == 0) continue;
	insert_surface(k[i], x * 2 + i % 2, y * 2 + (i / 2) % 2, z * 2 + (i / 4) % 2, lod + 1);
      }
    }
  }
  insert_surface(idx, 0, 0, 0, 0);
  logd("SIZE: %i %i\n", tab->key_area->size, sizeof(voxel_collider_key));
  
}


void gl_render_cube(mat4 cam, float cube_size, vec3 cube_position, u32 color){
  static bool initialized = false;
  static int shader = -1;
  static int camera_loc = -1;
  static int color_loc = -1;
  static u32 cube_vbo = 0;
  static u32 cube_ivbo = 0;
  static int vertex_loc = 0;
  if(!initialized && (initialized = true)){

    char * vs = read_file_to_string("voxel_shader.vs");
    char * fs = read_file_to_string("voxel_shader.fs");
    shader = load_simple_shader(vs, strlen(vs), fs, strlen(fs));
    dealloc(vs);
    dealloc(fs);
    camera_loc = glGetUniformLocation(shader, "camera");
    color_loc = glGetUniformLocation(shader, "color");
    vertex_loc = glGetAttribLocation(shader, "vertex");
    float xyz[] = {0,0,0, 1,0,0, 1,1,0, 0,1,0,
		   0,0,1, 1,0,1, 1,1,1, 0,1,1};
    u8 indexes[] = {4,3,2,1, 3,4,8,7, 2,3,7,6, 1,2,6,5, 5,8,4,1, 5,6,7,8};
    for(u32 i = 0; i < array_count(indexes); i++)
      indexes[i] -= 1;
    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(xyz), xyz, GL_STATIC_DRAW);

    glGenBuffers(1, &cube_ivbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ivbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);
  }
  glUseProgram(shader);
  mat4 s = mat4_scaled(cube_size, cube_size, cube_size);
  s.columns[3].xyz = cube_position;
  s.columns[3].w = 1;
  s = mat4_mul(cam, s);
  
  glUniformMatrix4fv(camera_loc,1,false, &(s.data[0][0]));
  vec4 c = u32_to_color(color);
  glUniform4f(color_loc, c.x, c.y, c.z, c.w);
  
  glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ivbo);
  glEnableVertexAttribArray(vertex_loc);
  glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, false, 0, (GLvoid*)0);
  glPointSize(3);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  //glDrawArrays(GL_QUADS, 0, 8);
  u64 drawn = 0;
  glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, (void *)drawn);
  //glDrawElements(GL_POINTS, 24, GL_UNSIGNED_BYTE, (void *)drawn);
}

void render_voxel_grid2(u64 id, u32 index, index_table * voxels, index_table * materials){
  UNUSED(materials);
  mat4 cam = get_camera_3d_position(id);
  
  void render_grid_inner(float size, vec3 offset, u32 index){
    //gl_render_cube(cam, size* 0.8, vec3_add(vec3_new1(size * 0.1), offset), 0xFF5555FF);
    u32 * indexes = lookup_index_table(voxels, index);
    float size2 = size * 0.5;
    for(int i = 0; i < 8; i++){
      vec3 offset2 = vec3_add(offset, vec3_mul(vec3_new1(size2), vec3_new(i % 2, (i / 2) % 2, (i / 4) % 2)));
      u32 index2 = indexes[i];
      if(index2 == 0)
	continue;
      if( index2 < index_table_count(voxels)){
	render_grid_inner(size2, offset2, index2);
      }else{

	u32 color = *((u32 *)lookup_index_table(materials, ~index2));
	gl_render_cube(cam, size2 * 0.8, vec3_add(vec3_new1(size2 * 0.1), offset2), color);
      }
    }
    
  }

  render_grid_inner(1.0, vec3_new(0,0,0), index);
}

void render_voxel_grid(u64 id){
  voxel_board_data data = get_board_data2(id);
  render_voxel_grid2(id, data.index, data.voxels, data.materials);
}

void measure_voxel_grid(u64 id, vec2 * size){
  UNUSED(id);
  *size = vec2_new(0, 0);
}
#include <sys/mman.h>
bool index_table_test(){
  test_simple_graphics();
  {
    board_data2_table * table = board_data2_table_create(NULL);
    logd("table size: %i\n", table->ptr->key_area->size);
    voxel_board_data bd = {.voxels = create_index_table("voxeltable2", sizeof(u32) * 8),
			   .materials = create_index_table("materials2", sizeof(u32)),
			   .index = 5};
    board_data2_set(table, 0xbeef, bd);

    auto bd2 = board_data2_get(table, 0xbeef);

    TEST_ASSERT(bd2.index == 5);
  }
  return TEST_SUCCESS;
  // the index table can be used for voxels.
  index_table * tab = create_index_table("voxeltable", sizeof(u32) * 8);
  index_table * colors = create_index_table("colortable", 4);
  u32 black = alloc_index_table(colors);
  u32 white = alloc_index_table(colors);
  printf("%p %p\n", black, white);
  *((u32 *) lookup_index_table(colors, black)) = 0x00FF00FF;
  *((u32 *)lookup_index_table(colors, white)) = 0xFFF00FFF;

  // Allocate 5 2x2x2 voxel chunks
  u32 x1 = alloc_index_table(tab);
  u32 x2 = alloc_index_table(tab);
  u32 x3 = alloc_index_table(tab);
  u32 x4 = alloc_index_table(tab);
  u32 x5 = alloc_index_table(tab);

  voxel_board_data vboard = {
    .index = x1,
    .materials = colors,
    .voxels = tab
  };

  UNUSED(x4), UNUSED(x5);
  u32 * d = lookup_index_table(tab, x1);
  // insert the IDs.
  d[0] = x2;
  d[1] = x3;
  d[3] = x2;
  d[6] = x2;
  d[2] = ~black;
  calc_voxel_surface(tab, x1);
  //  return TEST_SUCCESS;
  //lookup the arrays for x2 x3  
  u32 * d1 = lookup_index_table(tab, x2);
  //u32 * d2 = lookup_index_table(tab, x3);

  // put in black and white colors.
  for(int i = 0; i < 8; i++){
    if((rand() % 2) == 0) continue;
    //d1[i] = (rand()%2) == 0 ? ~white : ~black;
    //d2[i] = (rand()%2) == 1 ? ~white : ~black;
  }
  d1[7] = x4;
  d1[2] = x4;
  d1[0] = x4;
  u32 * d3 = lookup_index_table(tab, x4);
  d3[7] = x5;
  d3[2] = x5;
  d3[0] = x5;
  u32 * d4 = lookup_index_table(tab, x5);
  d4[0] = ~white;
  d4[7] = ~black;

  u32 count = iterate_voxel_chunk(tab, x1);
  logd("Count: %i\n", count);
  init_gui();
  u64 voxel_board = intern_string("voxel board");
  set_board_data2(voxel_board, vboard);

  mat4_print(get_camera_3d_position(voxel_board));
  define_method(voxel_board, render_control_method, (method)render_voxel_grid);
  define_method(voxel_board, measure_control_method, (method)measure_voxel_grid);

  u64 win_id = intern_string("voxel window");
  set_color(win_id, vec3_new(0.1, 0.1, 0.1));
  make_window(win_id); 
  add_control(win_id, voxel_board);
  auto method = get_method(win_id, render_control_method);
  double i = 0;
  while(true){
    mat4 p = mat4_perspective(0.8, 1, 0.1, 10);
    i += 0.03;
    mat4 t = mat4_rotate(mat4_translate(0.5,0.5,0),1,0.23,0.1,i);//mat4_rotate(mat4_translate(-0.0,-1.5,-4 + 0 * sin(i)),0,1,1, 0.0 * i);
    t = mat4_mul(t, mat4_translate(0,0,4));
    set_camera_3d_position(voxel_board, mat4_mul(p, mat4_invert(t)));
    //iron_sleep(0.01);
    method(win_id);
  }
  return TEST_SUCCESS;
}

int main(){
  //table2_test();
  //test_persist_oop();
  //test_walls();
  //TEST(test_elf_reader2);
  //TEST(test_elf_reader);
  

  
  if (!glfwInit())
    return -1;
  TEST(index_table_test);
  //test_gui();
  return 0;
}
