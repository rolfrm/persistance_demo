color_alpha * color_alpha_create(const char * optional_name){
  static const char * const column_names[] = {(char *)"key", (char *)"color"};
  static const char * const column_types[] = {"u64", "vec4"};
  color_alpha * instance = alloc0(sizeof(color_alpha));
  abstract_sorttable_init((abstract_sorttable * )instance, optional_name, 2, (u32[]){sizeof(u64), sizeof(vec4)}, (char *[]){(char *)"key", (char *)"color"});
  instance->column_names = (char **)column_names;
  instance->column_types = (char **)column_types;
  return instance;
}

void color_alpha_insert(color_alpha * table, u64 * key, vec4 * color, u64 count){
  void * array[] = {(void* )key, (void* )color};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, count);
}

void color_alpha_set(color_alpha * table, u64 key, vec4 color){
  void * array[] = {(void* )&key, (void* )&color};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, 1);
}

void color_alpha_lookup(color_alpha * table, u64 * keys, u64 * out_indexes, u64 count){
  abstract_sorttable_finds((abstract_sorttable *) table, keys, out_indexes, count);
}

void color_alpha_remove(color_alpha * table, u64 * keys, u64 key_count){
  u64 indexes[key_count];
  abstract_sorttable_finds((abstract_sorttable *) table, keys, indexes, key_count);
  abstract_sorttable_remove_indexes((abstract_sorttable *) table, indexes, key_count);
}

void color_alpha_clear(color_alpha * table){
  abstract_sorttable_clear((abstract_sorttable *) table);
}

void color_alpha_unset(color_alpha * table, u64 key){
  color_alpha_remove(table, &key, 1);
}

bool color_alpha_try_get(color_alpha * table, u64 * key, vec4 * color){
  void * array[] = {(void* )key, (void* )color};
  void * column_pointers[] = {(void *)table->key, (void *)table->color};
  u64 __index = 0;
  abstract_sorttable_finds((abstract_sorttable *) table, array[0], &__index, 1);
  if(__index == 0) return false;
  u32 sizes[] = {sizeof(u64), sizeof(vec4)};
  for(int i = 1; i < 2; i++){
    memcpy(array[i], column_pointers[i] + __index * sizes[i], sizes[i]); 
  }
  return true;
}

void color_alpha_print(color_alpha * table){
  abstract_sorttable_print((abstract_sorttable *) table);
}
