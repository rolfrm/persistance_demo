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
#include "simple_graphics.h"
#include "game_board.h"
void table2_test();
void test_walls();
void test_persist_oop();
bool test_elf_reader2();
bool test_elf_reader();
bool test_type_system();
void simple_grid_renderer_create(u64 id);
void simple_grid_initialize(u64 id);
bool copy_nth(const char * _str, u32 index, char * buffer, u32 buffer_size);
u32 index_table_capacity(index_table * table){
  return table->area->size / table->element_size - 4;
}

u32 index_table_count(index_table * table){
  return ((u32 *) table->area->ptr)[0];
}

void index_table_count_set(index_table * table, u32 newcount){
  ((u32 *) table->area->ptr)[0] = newcount;
}

u32 _index_table_free_index_count(index_table * table){
  return ((u32 *) table->free_indexes->ptr)[0];
}

void _index_table_free_index_count_set(index_table * table, u32 cnt){
  ((u32 *) table->free_indexes->ptr)[0] = cnt;
}
void * index_table_all(index_table * table, u64 * cnt){
  // return count -1 and a pointer from the placeholder element.
  *cnt = index_table_count(table) - 1;
  return table->area->ptr + table->element_size * 5;
}
void index_table_clear(index_table * table){
  index_table_count_set(table, 1);
  _index_table_free_index_count_set(table, 0);
}

u32 _index_table_alloc(index_table * table){
  u32 freeindexcnt = _index_table_free_index_count(table);
  if(freeindexcnt > 0){
    u32 idx = ((u32 *) table->free_indexes->ptr)[freeindexcnt];
    _index_table_free_index_count_set(table, freeindexcnt - 1);
    ASSERT(idx != 0);
    void * p = index_table_lookup(table, idx);
    memset(p, 0, table->element_size);
    return idx;
  }
  
  while(index_table_capacity(table) <= index_table_count(table)){
    u32 prevsize = table->area->size;
    ASSERT((prevsize % table->element_size) == 0);
    u32 newsize = MAX(prevsize * 2, 8 * table->element_size);
    mem_area_realloc(table->area, newsize);
    memset(table->area->ptr + prevsize, 0,  newsize - prevsize);
  }
  u32 idx = index_table_count(table);
  index_table_count_set(table,idx + 1);
  return idx;
}

u32 index_table_alloc(index_table * table){
  auto index = _index_table_alloc(table);
  ASSERT(index > 0);
  return index;
}


void index_table_remove(index_table * table, u32 index){
  ASSERT(index < index_table_count(table));
  ASSERT(index > 0);
  u32 cnt = _index_table_free_index_count(table);
  mem_area_realloc(table->free_indexes, table->free_indexes->size + sizeof(u32));
  ((u32 *)table->free_indexes->ptr)[cnt + 1] = 0;
  //ASSERT(memmem(table->free_indexes->ptr + sizeof(u32), (cnt + 1) * sizeof(u32), &index, sizeof(index)) == NULL);
  ((u32 *)table->free_indexes->ptr)[cnt + 1] = index;
  ((u32 *) table->free_indexes->ptr)[0] += 1;
}

void index_table_resize_sequence(index_table * table, index_table_sequence * seq,  u32 new_count){
  index_table_sequence nseq;
  if(new_count > 0)
    nseq = index_table_alloc_sequence(table, new_count);
  else{
    nseq.index = 0;
    nseq.count = 0;
  }
  if(seq->index != 0 && seq->count != 0 ){
    if(new_count > 0){
      void * src = index_table_lookup_sequence(table, *seq);
      void * dst = index_table_lookup_sequence(table, nseq);
      memmove(dst, src, MIN(seq->count, nseq.count) * table->element_size);
    }
    index_table_remove_sequence(table, seq);
  }
  *seq = nseq;
}

index_table_sequence index_table_alloc_sequence(index_table * table, u32 count){
  u32 freeindexcnt = _index_table_free_index_count(table);
  
  if(freeindexcnt > 0){
      u32 start = 0;
      u32 cnt = 0;
      for(u32 i = 0; i < freeindexcnt; i++){
	u32 idx = ((u32 *) table->free_indexes->ptr)[i + 1];
	if(start == 0){
	  start = idx;
	  cnt = 1;
	}else if(idx == start + cnt){
	  cnt += 1;
	}else if(idx == start + cnt - 1){
	  ASSERT(false);
	}else{
	  start = idx;
	  cnt = 1;
	}
	if(cnt == count){
	  // pop it from the indexex.
	  _index_table_free_index_count_set(table, freeindexcnt - cnt);
	  u32 * ptr = table->free_indexes->ptr;
	  for(u32 j = i - cnt + 1; j < freeindexcnt - cnt; j++)
	    ptr[j + 1] = ptr[j + cnt + 1];
	  
	  ASSERT(start != 0);
	  void * p = index_table_lookup(table, start);
	  memset(p, 0, cnt * table->element_size);
	  return (index_table_sequence){.index = start, .count = cnt};
	}
      }
    }
  
  while(index_table_capacity(table) <= (index_table_count(table) + count)){
    u32 prevsize = table->area->size;
    ASSERT((prevsize % table->element_size) == 0);
    u32 newsize = MAX(prevsize * 2, 8 * table->element_size);
    mem_area_realloc(table->area, newsize);
    memset(table->area->ptr + prevsize, 0,  newsize - prevsize);
  }
  u32 idx = index_table_count(table);
  index_table_count_set(table,idx + count);
  return (index_table_sequence){.index = idx, .count = count};
}

void index_table_remove_sequence(index_table * table, index_table_sequence * seq){
  u32 cnt = _index_table_free_index_count(table);
  mem_area_realloc(table->free_indexes, table->free_indexes->size + seq->count * sizeof(u32));
  //ASSERT(memmem(table->free_indexes->ptr + sizeof(u32), cnt * sizeof(u32), &index, sizeof(index)) == NULL);
  for(u32 i = 0; i < seq->count; i++)
    ((u32 *)table->free_indexes->ptr)[cnt + i + 1] = seq->index + i;
  ((u32 *) table->free_indexes->ptr)[0] += seq->count;
  memset(seq, 0, sizeof(*seq));
  index_table_optimize(table);
}

void * index_table_lookup_sequence(index_table * table, index_table_sequence seq){
  if(seq.index == 0)
    return NULL;
  return index_table_lookup(table, seq.index);
}


index_table * index_table_create(const char * name, u32 element_size){
  ASSERT(element_size > 0);
  index_table table = {0};
  table.element_size = element_size;
  if(name != NULL){

    table.area = mem_area_create(name);
    char name2[128] = {0};
    sprintf(name2, "%s.free", name);
    table.free_indexes = mem_area_create(name2);
  }else{
    table.area = mem_area_create_non_persisted();
    table.free_indexes = mem_area_create_non_persisted();
  }
  
  if(table.free_indexes->size < sizeof(u32)){
    mem_area_realloc(table.free_indexes, sizeof(u32));
    ((u32 *)table.free_indexes->ptr)[0] = 0;
  }

  if(table.area->size < element_size){
    mem_area_realloc(table.area, element_size * 4);
    memset(table.area->ptr, 0, table.area->size);
    _index_table_alloc(&table);
  }
  ASSERT((table.area->size % table.element_size) == 0);
  ASSERT(index_table_count(&table) > 0);
  return iron_clone(&table, sizeof(table));
}


void index_table_destroy(index_table ** _table){
  index_table * table = *_table;
  *_table = NULL;
  mem_area_free(table->area);
  mem_area_free(table->free_indexes);
}

void * index_table_lookup(index_table * table, u32 index){
  ASSERT(index < index_table_count(table));
  ASSERT(index > 0);
  return table->area->ptr + (4 + index) * table->element_size;
}

bool index_table_contains(index_table * table, u32 index){
  if(index == 0)
    return false;
  if (index >= index_table_count(table))
    return false;
  u32 freecnt = _index_table_free_index_count(table);
  u32 * start = table->free_indexes->ptr + sizeof(u32);
  for(u32 i = 0; i < freecnt; i++){
    if(start[i] == index)
      return false;
  }
  return true;
}

void index_table_optimize(index_table * table){
  int cmpfunc (const u32 * a, const u32 * b)
  {
    return ( *(int*)a - *(int*)b );
  }
  u32 cnt = _index_table_free_index_count(table);
  if(table->free_indexes->ptr == NULL || cnt ==  0)
    return;
    
  u32 * p = table->free_indexes->ptr;
  qsort(p + 1,cnt , sizeof(u32), (void *) cmpfunc);
  u32 icnt = index_table_count(table);
  
  u32 freedcount = 0;
  while(icnt > 0 && p[cnt] >= icnt){
    freedcount += 1;
    cnt -= 1;
    }
  while(icnt > 0 && p[cnt] == icnt - 1){

    icnt -= 1;
    cnt -= 1;
    freedcount += 1;
  }
  _index_table_free_index_count_set(table, cnt);
  index_table_count_set(table, icnt);
  u64 newsize = icnt * table->element_size + table->element_size * 5;
  mem_area_realloc(table->area, newsize);
  u64 newsize2 = cnt * table->element_size + sizeof(u32);
  mem_area_realloc(table->free_indexes, newsize2);
}


u32 iterate_voxel_chunk(index_table * table, u32 start_index){
  u32 count = 0;
  u32 * ref = index_table_lookup(table, start_index);
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
      u32 * k = index_table_lookup(vox, idx2);
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
    //gl_render_cube(cam, size* 0.8, vec3_add(vec3_new1(size * 0.1),
//offset), 0xFF5555FF);
    if(index == 0)
      return;
    u32 * indexes = index_table_lookup(voxels, index);
    float size2 = size * 0.5;
    for(int i = 0; i < 8; i++){
      vec3 offset2 = vec3_add(offset, vec3_mul(vec3_new1(size2), vec3_new(i % 2, (i / 2) % 2, (i / 4) % 2)));
      u32 index2 = indexes[i];
      if(index2 == 0)
	continue;
      if( index2 < index_table_count(voxels)){
	render_grid_inner(size2, offset2, index2);
      }else{

	u32 color = *((u32 *)index_table_lookup(materials, ~index2));
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
  { // copy nth
    //bool copy_nth(string str, u32 index, char * buffer, u32 buffer_size)
    const char * teststr = "  hello   world   2";
    const char * cmp[] = {"hello", "world", "2"};
    char buffer[10];
    for(int i = 0; i < 3; i++){
      copy_nth(teststr, i, buffer, array_count(buffer));
      ASSERT(strcmp(buffer, cmp[i]) == 0);
    }

  }
  
  if(true){// first test.
    for(int k = 1; k < 5;k++){
      logd("K: %i\n", k);
      index_table * t = index_table_create(NULL, sizeof(i64));
      index_table_optimize(t);
      index_table_sequence seq = index_table_alloc_sequence(t, 20);
      index_table_sequence seq3 = index_table_alloc_sequence(t, 10);      
      index_table_resize_sequence(t, &seq, 9);
      index_table_remove_sequence(t, &seq3);

      index_table_remove_sequence(t, &seq);      
      index_table_optimize(t);

      
      seq = index_table_alloc_sequence(t, 20);
      logd("%i \n", _index_table_free_index_count(t));
      ASSERT(_index_table_free_index_count(t) == 0);
      //u32 i1 = seq.index;
      ASSERT(seq.count == 20);
      ASSERT(seq.index > 0);
      i64 * pts = index_table_lookup_sequence(t, seq);
      for(int i = 0; i < 20; i++)
	pts[i] = i;
      index_table_resize_sequence(t, &seq, 21);

      seq3 = index_table_alloc_sequence(t, 10);
      index_table_sequence seq4 = index_table_alloc_sequence(t, 40);
      ASSERT(seq.count == 21);
      pts = index_table_lookup_sequence(t, seq);
      for(int i = 0; i < 20; i++){
	ASSERT(pts[i] == i);
      }
     
      memset(pts,255, sizeof(i64) * 21);
      index_table_remove_sequence(t, &seq);
      logd("Free count: %i\n", _index_table_free_index_count(t));
      index_table_sequence seq2 = index_table_alloc_sequence(t, 20);
      //ASSERT(seq2.index == i1);
      index_table_remove_sequence(t, &seq2);
      index_table_remove_sequence(t, &seq3);
      index_table_remove_sequence(t, &seq4); 
      index_table_optimize(t);
      logd("Free count: %i\n", _index_table_free_index_count(t));
      ASSERT(_index_table_free_index_count(t) == 0);
      u32 idxes[200];
      for(u32 j = 3; j < 20; j++){
	
	for(u32 i = 0; i < j * 10; i++)
	  idxes[i] = index_table_alloc(t);
	for(u32 i = 0; i < j * 10; i++)
	  ((u32 *) index_table_lookup(t, idxes[i]))[0] = 0xFFDDCCAA * k;
	for(u32 i = 0; i < j * 10; i++)
	  ASSERT(((u32 *) index_table_lookup(t, idxes[i]))[0] == 0xFFDDCCAA * k);
	ASSERT(_index_table_free_index_count(t) == 0);
	for(u32 i = 0; i < j * 10; i++)
	  index_table_remove(t, idxes[i]);
      }
      index_table_optimize(t);
      ASSERT(index_table_count(t) == 1);
      index_table_destroy(&t);
    }
  }
  
  if(false){
    
    board_data2_table * table = board_data2_table_create(NULL);
    voxel_board_data bd = {.voxels = index_table_create("voxeltable2", sizeof(u32) * 8),
			   .materials = index_table_create("materials2", sizeof(u32)),
			   .index = 5};
    board_data2_set(table, 0xbeef, bd);

    auto bd2 = board_data2_get(table, 0xbeef);

    TEST_ASSERT(bd2.index == 5);
  }


  
  //return TEST_SUCCESS;
  // the index table can be used for voxels.
  if(false){
    index_table * tab = index_table_create("voxeltable", sizeof(u32) * 8);
    index_table * colors = index_table_create("colortable", 4);
    u32 black = index_table_alloc(colors);
    u32 white = index_table_alloc(colors);
    *((u32 *) index_table_lookup(colors, black)) = 0x00FF00FF;
    *((u32 *)index_table_lookup(colors, white)) = 0xFFF00FFF;

    // Allocate 5 2x2x2 voxel chunks
    u32 x1 = index_table_alloc(tab);
    u32 x2 = index_table_alloc(tab);
    u32 x3 = index_table_alloc(tab);
    u32 x4 = index_table_alloc(tab);
    u32 x5 = index_table_alloc(tab);
    
    voxel_board_data vboard = {
      .index = x1,
      .materials = colors,
      .voxels = tab
    };
    
    UNUSED(x4), UNUSED(x5);
    u32 * d = index_table_lookup(tab, x1);
    // insert the IDs.
    d[0] = x2;
    d[1] = x3;
    d[3] = x2;
    d[6] = x2;
    d[2] = ~black;
    calc_voxel_surface(tab, x1);
    //  return TEST_SUCCESS;
    //lookup the arrays for x2 x3  
    u32 * d1 = index_table_lookup(tab, x2);
    //u32 * d2 = index_table_lookup(tab, x3);
    
    // put in black and white colors.
    for(int i = 0; i < 8; i++){
      if((rand() % 2) == 0) continue;
      //d1[i] = (rand()%2) == 0 ? ~white : ~black;
      //d2[i] = (rand()%2) == 1 ? ~white : ~black;
    }
    d1[7] = x4;
    d1[2] = x4;
    d1[0] = x4;
    u32 * d3 = index_table_lookup(tab, x4);
    d3[7] = x5;
    d3[2] = x5;
    d3[0] = x5;
    u32 * d4 = index_table_lookup(tab, x5);
    d4[0] = ~white;
    d4[7] = ~black;
    
    u32 count = iterate_voxel_chunk(tab, x1);
    logd("Count: %i\n", count);

    //TEST(simple_graphics_editor_test);
    u64 voxel_board = intern_string("voxel board");
    set_board_data2(voxel_board, vboard);
    
    mat4_print(get_camera_3d_position(voxel_board));
  }
  init_gui();
  u64 game_board = intern_string("voxel board");
  u64 win_id = intern_string("voxel window");
  u64 fpstextline = 0;
  {
    fpstextline = intern_string("text_line_1");
    get_textline(fpstextline);
    add_control(win_id, fpstextline);
    set_horizontal_alignment(fpstextline, HALIGN_LEFT);
    set_vertical_alignment(fpstextline, VALIGN_TOP);
  }
  
  simple_graphics_editor_load(game_board, win_id);
  make_window(win_id);

  if(once(win_id + 0xFFFFFFFF134123)){
    set_color(win_id, vec3_new(1, 1, 1));
  }
  
  //add_control(win_id, voxel_board);
  auto method = get_method(win_id, render_control_method);
  double i = 0;
  {
    void * ptrs[10];
    for(u32 j = 0; j < array_count(ptrs); j++)
      ptrs[j] = alloc((j + 1) * 100);
    for(u32 j = 0; j < array_count(ptrs); j++)
      dealloc(ptrs[j]);
  }
  while(!get_should_exit(game_board)){
    u64 ts = timestamp(); // for fps calculation.
    mat4 p = mat4_perspective(0.8, 1, 0.1, 10);
    i += 0.03;
    mat4 t = mat4_rotate(mat4_translate(0.5,0.5,0),1,0.23,0.1,i);//mat4_rotate(mat4_translate(-0.0,-1.5,-4 + 0 * sin(i)),0,1,1, 0.0 * i);
    t = mat4_mul(t, mat4_translate(0,0,4));
    set_camera_3d_position(game_board, mat4_mul(p, mat4_invert(t)));
    //iron_sleep(0.03);
    glfwPollEvents();
    u64 ts2 = timestamp(); // for fps calculation.
    method(win_id);
    {
      u64 tsend = timestamp();
      u64 msdiff = tsend - ts;
      char fpsbuf[100];
      sprintf(fpsbuf, "%3.0f / %3.0f FPS", 1.0 / (1e-6 * (tsend - ts2)), 1.0 / (1e-6 * msdiff));
      remove_text(fpstextline);
      set_text(fpstextline, fpsbuf);
    }
	
    
  }
  unset_should_exit(game_board);
  return TEST_SUCCESS;
}

typedef struct{
  void (*f)();
  coroutine * cc;
  void * channels[10];
  u32 active;

}task;

typedef struct{
  float x, y, radius;
  u32 * channel;
}circle_body;

CREATE_TABLE_DECL2(circle_body, u32, circle_body);
CREATE_TABLE_NP(circle_body, u32, circle_body);

u32 circle_body_register(){
  return get_unique_number();
}

void circle_body_update(u32 id, float x, float y, float radius, void * channel){
  set_circle_body(id, (circle_body){.x = x, .y = y, .radius = radius, .channel = channel});
}

__thread coroutine * current_coroutine = NULL;

task * current_task = NULL;
int choose(void ** out){
  while(true){
    for(u32 i = 0; i < current_task->active; i++){
     
      if(current_task->channels[i] != NULL){
	*out = current_task->channels[i];
	current_task->channels[i] = NULL;
	return i;
      }
    }
    ccyield();
  }
  return 0;
}

void task_step(task * _task, bool channels){
  task * prev_task = current_task;
  current_task = _task;

  for(u32 i = 0; i < _task->active; i++){
    if(_task->channels[i] != NULL){
      if(_task->cc != NULL){
	ccstep(_task->cc);
	return;
      }
    }
  }
  if(channels){
    return;
    }
  UNUSED(channels);
  if(_task->f != NULL){
    _task->cc = ccstart(_task->f);
    _task->f = NULL;
  }else
    ccstep(_task->cc);
  
  current_task = prev_task;
}

void testcc1(){
  float x = 0, y = 0, radius = 1;
  void * result;
  u32 body = get_unique_number();
  circle_body_update(body, x, y, radius, current_task->channels + 0);
  current_task->active = 1;
  logd("Body updated..\n");
  while(true){
    u32 i = choose(&result);

    switch(i){
    case 0:
      x += (randf32() * 0.01 - 0.005) * 30;
      y += (randf32() * 0.01 - 0.005) * 30;
      logd("P: %f %f\n", x, y);
      circle_body_update(body, x, y, radius, current_task->channels + 0);
      break;
    case 1:
      logd("AI\n");
      break;
    default:
      logd("Dont know..\n");
    }
    ccyield();
  }
}

void check_collisions(){
  circle_body bodies[10] = {0};
  u32 body_ids[10] = {0};
  u64 idx = 0;
  u32 cnt = iter_all_circle_body(body_ids, bodies, array_count(bodies), &idx);
  for(u32 i = 0; i < cnt; i++){
    for(u32 j = i + 1; j < cnt; j++){
      circle_body a = bodies[i];
      circle_body b =  bodies[j];
      float dx = a.x - b.x;
      float dy = a.y - b.y;
      float d = sqrt(dx * dx + dy * dy);
      if(a.radius + b.radius > d){
	if(bodies[i].channel != NULL)
	  *bodies[i].channel = body_ids[j];
	if(bodies[j].channel != NULL)
	  *bodies[j].channel = body_ids[i];
      }
    }
  }
}

#include <iron/time.h>

void test_coroutines(){
  clear_circle_body();
  task * t1 = alloc0(sizeof(task));
  task * t2 = alloc0(sizeof(task));
  t1->f = testcc1;
  t2->f = testcc1;
  task_step(t1,false);
  task_step(t2, false);
  int rounds = 0;
 loop:
  rounds++;
  check_collisions();
  task_step(t1, true);
  task_step(t2, true);
  if(rounds % 1000000 == 0){
    logd("Round.. %i\n", rounds);
  }
  goto loop;
}

int iteration = 0;
void test_tsk2(){
  while(true){
    iteration += 1;
    ccyield();
  }
}
void test_coroutines2(){
  const u64 items =1000000;
  coroutine ** ccs = alloc0(items * sizeof(coroutine *));
  for(u64 i = 0; i < items; i++){
    ccs[i] = ccstart(test_tsk2); 
  }
  logd("Start..\n");
  for(int j = 0; j < 20; j++){
    for(u64 i = 0; i < items; i++){
      ccstep(ccs[i]);
    }
  }

  logd("IT: %i\n", iteration);

}

void test_hydra();
#include "abstract_sortable.h"
#include "MyTableTest.h"
#include "MyTableTest.c"
#include "is_node.h"
#include "is_node.c"

bool test_abstract_sortable(){

  is_node * table = is_node_create(NULL);
  for(u32 key = 100; key < 200; key+=2){
    logd("ITERATION: %i\n", key);
    is_node_set(table, key);
    for(u32 i = 0; i < table->count + 1; i++){
      logd("key %i %i\n", i, table->index[i]);
    }
    ASSERT(is_node_try_get(table, &key));
  }
  
  for(u32 key = 100; key < 200; key++){
    if(key%2 == 0){
      ASSERT(is_node_try_get(table, &key));
    }else{
      ASSERT(false == is_node_try_get(table, &key));
    }
  }

  MyTableTest * myTable = MyTableTest_create("MyTable");
  MyTableTest_clear(myTable);
  for(int j = 0; j < 2; j++){
    for(int i = 0; i < 40; i++){
      u64 key = i * 2;
      f32 x = sin(0.1 * key);
      f32 y = cos(0.1 * key);
      void * values[] = {(void *)&key, (void *)&x, (void *)&y};
      abstract_sorttable_inserts((abstract_sorttable *)myTable, values, 1);
    }
  }
  
  logd("COUNT: %i\n", myTable->count);
  ASSERT(myTable->count == 40);
  bool test = true;
  for(u32 i = 0; i < myTable->count; i++){
    if(test){
      u64 key = i * 2;
      f32 x = sin(0.1 * key);
      f32 y = cos(0.1 * key);
      ASSERT(myTable->index[i + 1] == key);
      ASSERT(myTable->x[i + 1] == x);
      ASSERT(myTable->y[i + 1] == y);
    }else{
      logd("%i %f %f\n", myTable->index[i + 1], myTable->x[i + 1], myTable->y[i + 1]);
    }
  }
  u64 keys_to_remove[] = {8, 10, 12};
  u64 indexes_to_remove[array_count(keys_to_remove)];
  
  abstract_sorttable_finds((abstract_sorttable *) myTable, keys_to_remove, indexes_to_remove, array_count(keys_to_remove));
  abstract_sorttable_remove_indexes((abstract_sorttable *) myTable, indexes_to_remove, array_count(keys_to_remove));
  myTable->count = myTable->index_area->size / myTable->sizes[0] - 1;
  ASSERT(myTable->count == 40 - array_count(keys_to_remove));
  
  for(u32 i = 0; i < myTable->count; i++){
    if(test){
      ASSERT(myTable->index[i + 1] != 8 && myTable->index[i + 1] != 10 && myTable->index[i + 1] != 12);
      u32 _i = i * 2;
      if(_i >= 8)
	_i = i * 2 + 6;
      f32 x = sin(0.1 * _i);
      f32 y = cos(0.1 * _i);
      ASSERT(myTable->x[i + 1] == x);
      ASSERT(myTable->y[i + 1] == y);
    }
  }
  ASSERT(myTable->index[20] > 0);
  MyTableTest_set(myTable, 100, 4.5, 6.0);
  ASSERT(myTable->index[20] > 0);
  u64 key = 100, index = 0;
  MyTableTest_lookup(myTable, &key, &index, 1);
  TEST_ASSERT(index != 0);
  logd("%i %f %f\n", index, myTable->x[index], myTable->y[index]);
  TEST_ASSERT(myTable->x[index] == 4.5 && myTable->y[index] == 6.0);
  u64 idx = 20;
  f32 x;
  f32 y;
  ASSERT(MyTableTest_try_get(myTable, &idx, &x, &y));
  logd("%i %f %f\n", idx, x, y);
  idx = 21;
  ASSERT(MyTableTest_try_get(myTable, &idx, &x, &y) == false);
  MyTableTest_print(myTable);
  return TEST_SUCCESS;

}
//int main2();
int main(int argc, char ** argv){
  if(argc == 2){
    logd("Setting data dir: %s\n", argv[1]);
    mem_area_set_data_directory(argv[1]);
  }
  //test_coroutines2();
  //return 0;
  //test_hydra();
  //return 0;
  //test_coroutines();
  //return 0;
  
  //void test_hydra();
  //test_hydra();
  //return 0;
  
  //table2_test();
  test_persist_oop();
  test_walls();
  //TEST(test_elf_reader2);
  //TEST(test_elf_reader);
  TEST(test_abstract_sortable);

  //return 0;
  
  
  if (!glfwInit())
    return -1;
  //gui_demo();

  TEST(index_table_test);
  //test_gui();
  return 0;
}
