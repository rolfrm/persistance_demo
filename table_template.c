TABLE_NAME * TABLE_NAME_create(const char * optional_name){
  TABLE_NAME * instance = alloc0(sizeof(TABLE_NAME));
  abstract_sorttable_init((abstract_sorttable * )instance, optional_name, COLUMN_COUNT, (u32[])COLUMN_SIZES, (char *[])COLUMN_NAMES);
  return instance;
}

void TABLE_NAME_insert(TABLE_NAME * table, VALUE_COLUMNS3, u64 count){
  void * array[] = {VALUE_PTRS2};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, count);
}

void TABLE_NAME_set(TABLE_NAME * table, VALUE_COLUMNS2){
  void * array[] = {VALUE_PTRS1};
  abstract_sorttable_inserts((abstract_sorttable *) table, array, 1);
}

void TABLE_NAME_lookup(TABLE_NAME * table, INDEX_TYPE * keys, u64 * out_indexes, u64 count){
  abstract_sorttable_finds((abstract_sorttable *) table, keys, out_indexes, count);
}

void TABLE_NAME_remove(TABLE_NAME * table, INDEX_TYPE * keys, u64 key_count){
  u64 indexes[key_count];
  abstract_sorttable_finds((abstract_sorttable *) table, keys, indexes, key_count);
  abstract_sorttable_remove_indexes((abstract_sorttable *) table, indexes, key_count);
}

void TABLE_NAME_clear(TABLE_NAME * table){
  abstract_sorttable_clear((abstract_sorttable *) table);
}

void TABLE_NAME_unset(TABLE_NAME * table, INDEX_TYPE key){
  TABLE_NAME_remove(table, &key, 1);
}

bool TABLE_NAME_try_get(TABLE_NAME * table, VALUE_COLUMNS3){
  void * array[] = {VALUE_PTRS2};
  void * column_pointers[] = COLUMN_POINTERS;
  u64 __index = 0;
  abstract_sorttable_finds((abstract_sorttable *) table, array[0], &__index, 1);
  if(__index == 0) return false;
  u32 sizes[] = COLUMN_SIZES;
  for(int i = 1; i < COLUMN_COUNT; i++){
    memcpy(array[i], column_pointers[i] + __index * sizes[i], sizes[i]); 
  }
  return true;
}

const char * const TABLE_NAME_column_types[] = COLUMN_TYPE_NAMES;
void TABLE_NAME_print(TABLE_NAME * table){
  void * column_pointers[] = COLUMN_POINTERS;
  u32 sizes[] = COLUMN_SIZES;
  for(u32 i = 0; i < COLUMN_COUNT;i++)
    logd("%s ", TABLE_NAME_column_types[i]);
  logd("\n");
  for(u32 i = 0; i < table->count; i++){
    for(u32 j = 0; j < COLUMN_COUNT;j++){
      table_print_cell(column_pointers[j] + (1 + i) * sizes[j], TABLE_NAME_column_types[j]);
      logd(" ");
    }
    logd("\n");
  }
}
