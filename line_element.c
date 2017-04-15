// This file is auto generated by table_compiler
line_element * line_element_create(const char * optional_name){
  static const char * const column_names[] = {(char *)"index", (char *)"curve_data"};
  static const char * const column_types[] = {"u32", "index_table_sequence"};
  line_element * instance = alloc0(sizeof(line_element));
  abstract_sorttable_init((abstract_sorttable * )instance, optional_name, 2, (u32[]){sizeof(u32), sizeof(index_table_sequence)}, (char *[]){(char *)"index", (char *)"curve_data"});
  instance->column_names = (char **)column_names;
  instance->column_types = (char **)column_types;
  return instance;
}

void line_element_insert(line_element * table, u32 * index, index_table_sequence * curve_data, u64 count){
  void * array[] = {(void* )index, (void* )curve_data};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, count);
}

void line_element_set(line_element * table, u32 index, index_table_sequence curve_data){
  void * array[] = {(void* )&index, (void* )&curve_data};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, 1);
}

void line_element_lookup(line_element * table, u32 * keys, u64 * out_indexes, u64 count){
  abstract_sorttable_finds((abstract_sorttable *) table, keys, out_indexes, count);
}

void line_element_remove(line_element * table, u32 * keys, u64 key_count){
  u64 indexes[key_count];
  abstract_sorttable_finds((abstract_sorttable *) table, keys, indexes, key_count);
  abstract_sorttable_remove_indexes((abstract_sorttable *) table, indexes, key_count);
}

void line_element_clear(line_element * table){
  abstract_sorttable_clear((abstract_sorttable *) table);
}

void line_element_unset(line_element * table, u32 key){
  line_element_remove(table, &key, 1);
}

bool line_element_try_get(line_element * table, u32 * index, index_table_sequence * curve_data){
  void * array[] = {(void* )index, (void* )curve_data};
  void * column_pointers[] = {(void *)table->index, (void *)table->curve_data};
  u64 __index = 0;
  abstract_sorttable_finds((abstract_sorttable *) table, array[0], &__index, 1);
  if(__index == 0) return false;
  u32 sizes[] = {sizeof(u32), sizeof(index_table_sequence)};
  for(int i = 1; i < 2; i++){
    if(column_pointers[i] != NULL)
      memcpy(array[i], column_pointers[i] + __index * sizes[i], sizes[i]); 
  }
  return true;
}

void line_element_print(line_element * table){
  abstract_sorttable_print((abstract_sorttable *) table);
}