TABLE_NAME * TABLE_NAME_create(const char * optional_name){
  TABLE_NAME * instance = alloc0(sizeof(TABLE_NAME));
  abstract_sortable * table = (abstract_sortable * )instance;
  ((int *)&table->column_count) = COLUMN_COUNT;
  abstract_sortable_init(table);
}

void TABLE_NAME_insert(TABLE_NAME * table, VALUE_COLUMNS3, u64 count){
  void ** array[] = VALUE_PTRS2;
  abstract_sortable_inserts((abstract_sortable *) table, array, count);
}

void TABLE_NAME_set(TABLE_NAME * table, VALUE_COLUMNS2){
  void ** array[] = VALUE_PTRS1;
  abstract_sortable_inserts((abstract_sortable *) table, array, 1);
}

void TABLE_NAME_lookup(TABLE_NAME * table, INDEX_TYPE * keys, u64 * out_indexes, u64 count){
  abstract_sortable_finds((abstract_sortable *) table, keys, out_indexes, count);
}
void TABLE_NAME_remove(TABLE_NAME * table, INDEX_TYPE * keys, u64 key_count){
  u64 indexes[key_count);
  abstract_sortable_finds((abstract_sortable *) table, keys, indexes, key_count);
  abstract_sortable_remove_indexes((abstract_sortable *) table, indexes, key_count);
}

void TABLE_NAME_clear(TABLE_NAME * table){
  abstract_sortable_clear((abstract_sortable *) table);
}

void TABLE_NAME_unset(TABLE_NAME * table, INDEX_TYPE key){
  TABLE_NAME_remove(table, &key, 1);
}
