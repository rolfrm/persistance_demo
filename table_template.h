typedef struct _TABLE_NAME{
  u64 count;
  const bool is_multi_table;
  const int column_count;
  int (*cmp) (const INDEX_TYPE * k1, const INDEX_TYPE * k2);
  const u64 sizes[COLUMN_COUNT];

  VALUE_COLUMNS1;
  MEM_AREAS;
}TABLE_NAME;

TABLE_NAME * TABLE_NAME_create(const char * optional_name);
void TABLE_NAME_set(TABLE_NAME * table, VALUE_COLUMNS2);
void TABLE_NAME_insert(TABLE_NAME * table, VALUE_COLUMNS3, u64 count);
void TABLE_NAME_lookup(TABLE_NAME * table, INDEX_TYPE * keys, u64 * out_indexes, u64 count);
void TABLE_NAME_remove(TABLE_NAME * table, INDEX_TYPE * keys, u64 key_count);
void TABLE_NAME_get_refs(TABLE_NAME * table, INDEX_TYPE * keys, u64 ** indexes, u64 count);
void TABLE_NAME_clear(TABLE_NAME * table);
void TABLE_NAME_unset(TABLE_NAME * table, INDEX_TYPE key);
bool TABLE_NAME_try_get(TABLE_NAME * table, VALUE_COLUMNS3);
void TABLE_NAME_print(TABLE_NAME * table);

