
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

void table2_test();
void test_walls();
void test_persist_oop();
bool test_elf_reader2();
bool test_elf_reader();
bool test_type_system();

typedef struct{
  void * ptr;
  u32 capacity;
  u32 count;

  u32 * free_indexes;
  u32 free_index_count;

  u32 element_size;
}index_table;

index_table * create_index_table(const char * name, u32 element_size){
  index_table table = {0};
  table.element_size = element_size;
  table.ptr = persist_alloc(name, 0);
  table.capacity = 0;
  table.count = 0;
  
  char name2[128];
  sprintf(name2, "%s.free", name);
  
  table.free_indexes = persist_alloc(name2, 0);
  table.free_index_count = 0;
  return iron_clone(&table, sizeof(table));
}

u32 alloc_index_table(index_table * table){
  if(table->free_index_count > 0){
    u32 idx = table->free_indexes[table->free_index_count -1];
    table->free_index_count -= 1;
    return idx;
  }
  while(table->capacity <= table->count){
    u32 prevsize = table->capacity * table->element_size;
    table->ptr = persist_realloc(table->ptr, MAX(table->capacity * 2, (u32)8) * table->element_size);
    table->capacity = MAX(table->capacity * 2, (u32)8);
    u32 newsize = table->capacity * table->element_size;
    memset(table->ptr + prevsize, 0, newsize - prevsize);
  }
  u32 idx = table->count++;
  return idx;
}

void * lookup_index_table(index_table * table, u32 index){
  ASSERT(index < table->count);
  return table->ptr + index * table->element_size;
}

u32 iterate_voxel_chunk(index_table * table, u32 start_index){
  u32 count = 0;
  u32 * ref = lookup_index_table(table, start_index);
  for(u32 i = 0; i < 8; i++){
    u32 data = ref[i];
    if(data == 0) continue;
    if(data > table->count){
      u32 color = ~data;
      logd("#%p ", color);
      count++;
    }else{
      count += iterate_voxel_chunk(table, data);
    }
  }
  return count;
}

bool index_table_test(){
  // the index table can be used for voxels.
  index_table * tab = create_index_table("voxeltable", sizeof(u32) * 8);
  index_table * colors = create_index_table("colortable", 4);
  alloc_index_table(colors);  alloc_index_table(colors);  alloc_index_table(colors);
  u32 black = alloc_index_table(colors);
  u32 white = alloc_index_table(colors);
  printf("%p %p\n", black, white);
  *((u32 *) lookup_index_table(colors, black)) = 0x000000FF;
  *((u32 *)lookup_index_table(colors, white)) = 0xFFFFFFFF;

  // Allocate 5 2x2x2 voxel chunks
  alloc_index_table(tab); // the null chunk
  u32 x1 = alloc_index_table(tab);
  u32 x2 = alloc_index_table(tab);
  u32 x3 = alloc_index_table(tab);
  u32 x4 = alloc_index_table(tab);
  u32 x5 = alloc_index_table(tab);
  UNUSED(x4), UNUSED(x5);
  u32 * d = lookup_index_table(tab, x1);
  // insert the IDs.
  d[0] = x2;
  d[1] = x3;
  
  //lookup the arrays for x2 x3  
  u32 * d1 = lookup_index_table(tab, x2);
  u32 * d2 = lookup_index_table(tab, x3);

  // put in black and white colors.
  for(int i = 0; i < 8; i++){
    d1[i] = (i%2) == 0 ? ~white : ~black;
    d2[i] = (i%2) == 1 ? ~white : ~black;
  }

  u32 count = iterate_voxel_chunk(tab, x1);
  logd("Count: %i\n", count);
  return TEST_SUCCESS;
}
int main(){
  //table2_test();
  //test_persist_oop();
  //test_walls();
  //TEST(test_elf_reader2);
  //TEST(test_elf_reader);
  
  TEST(index_table_test);
  
  if (!glfwInit())
    return -1;
  //test_gui();
  return 0;
}
