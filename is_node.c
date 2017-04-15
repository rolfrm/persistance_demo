is_node * is_node_create(const char * optional_name){
  static const char * const column_names[] = {(char *)"index"};
  static const char * const column_types[] = {"u32"};
  is_node * instance = alloc0(sizeof(is_node));
  abstract_sorttable_init((abstract_sorttable * )instance, optional_name, 1, (u32[]){sizeof(u32)}, (char *[]){(char *)"index"});
  instance->column_names = (char **)column_names;
  instance->column_types = (char **)column_types;
  return instance;
}

void is_node_insert(is_node * table, u32 * index, u64 count){
  void * array[] = {(void* )index};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, count);
}

void is_node_set(is_node * table, u32 index){
  void * array[] = {(void* )&index};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, 1);
}

void is_node_lookup(is_node * table, u32 * keys, u64 * out_indexes, u64 count){
  abstract_sorttable_finds((abstract_sorttable *) table, keys, out_indexes, count);
}

void is_node_remove(is_node * table, u32 * keys, u64 key_count){
  u64 indexes[key_count];
  abstract_sorttable_finds((abstract_sorttable *) table, keys, indexes, key_count);
  abstract_sorttable_remove_indexes((abstract_sorttable *) table, indexes, key_count);
}

void is_node_clear(is_node * table){
  abstract_sorttable_clear((abstract_sorttable *) table);
}

void is_node_unset(is_node * table, u32 key){
  is_node_remove(table, &key, 1);
}

bool is_node_try_get(is_node * table, u32 * index){
  void * array[] = {(void* )index};
  void * column_pointers[] = {(void *)table->index};
  u64 __index = 0;
  abstract_sorttable_finds((abstract_sorttable *) table, array[0], &__index, 1);
  if(__index == 0) return false;
  u32 sizes[] = {sizeof(u32)};
  for(int i = 1; i < 1; i++){
    memcpy(array[i], column_pointers[i] + __index * sizes[i], sizes[i]); 
  }
  return true;
}

void is_node_print(is_node * table){
  abstract_sorttable_print((abstract_sorttable *) table);
}
