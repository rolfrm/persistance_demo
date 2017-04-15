MyTableTest * MyTableTest_create(const char * optional_name){
  static const char * const column_names[] = {(char *)"index", (char *)"x", (char *)"y"};
  static const char * const column_types[] = {"u64", "f32", "f32"};
  MyTableTest * instance = alloc0(sizeof(MyTableTest));
  abstract_sorttable_init((abstract_sorttable * )instance, optional_name, 3, (u32[]){sizeof(u64), sizeof(f32), sizeof(f32)}, (char *[]){(char *)"index", (char *)"x", (char *)"y"});
  instance->column_names = (char **)column_names;
  instance->column_types = (char **)column_types;
  return instance;
}

void MyTableTest_insert(MyTableTest * table, u64 * index, f32 * x, f32 * y, u64 count){
  void * array[] = {(void* )index, (void* )x, (void* )y};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, count);
}

void MyTableTest_set(MyTableTest * table, u64 index, f32 x, f32 y){
  void * array[] = {(void* )&index, (void* )&x, (void* )&y};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, 1);
}

void MyTableTest_lookup(MyTableTest * table, u64 * keys, u64 * out_indexes, u64 count){
  abstract_sorttable_finds((abstract_sorttable *) table, keys, out_indexes, count);
}

void MyTableTest_remove(MyTableTest * table, u64 * keys, u64 key_count){
  u64 indexes[key_count];
  abstract_sorttable_finds((abstract_sorttable *) table, keys, indexes, key_count);
  abstract_sorttable_remove_indexes((abstract_sorttable *) table, indexes, key_count);
}

void MyTableTest_clear(MyTableTest * table){
  abstract_sorttable_clear((abstract_sorttable *) table);
}

void MyTableTest_unset(MyTableTest * table, u64 key){
  MyTableTest_remove(table, &key, 1);
}

bool MyTableTest_try_get(MyTableTest * table, u64 * index, f32 * x, f32 * y){
  void * array[] = {(void* )index, (void* )x, (void* )y};
  void * column_pointers[] = {(void *)table->index, (void *)table->x, (void *)table->y};
  u64 __index = 0;
  abstract_sorttable_finds((abstract_sorttable *) table, array[0], &__index, 1);
  if(__index == 0) return false;
  u32 sizes[] = {sizeof(u64), sizeof(f32), sizeof(f32)};
  for(int i = 1; i < 3; i++){
    memcpy(array[i], column_pointers[i] + __index * sizes[i], sizes[i]); 
  }
  return true;
}

void MyTableTest_print(MyTableTest * table){
  abstract_sorttable_print((abstract_sorttable *) table);
}
