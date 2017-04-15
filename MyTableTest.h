typedef struct _MyTableTest{
  char ** column_names;
  char ** column_types;
  u64 count;
  const bool is_multi_table;
  const u32 column_count;
  int (*cmp) (const u64 * k1, const u64 * k2);
  const u64 sizes[3];

  u64 * index;
  f32 * x;
  f32 * y;
  mem_area * index_area;
  mem_area * x_area;
  mem_area * y_area;
}MyTableTest;

MyTableTest * MyTableTest_create(const char * optional_name);
void MyTableTest_set(MyTableTest * table, u64 index, f32 x, f32 y);
void MyTableTest_insert(MyTableTest * table, u64 * index, f32 * x, f32 * y, u64 count);
void MyTableTest_lookup(MyTableTest * table, u64 * keys, u64 * out_indexes, u64 count);
void MyTableTest_remove(MyTableTest * table, u64 * keys, u64 key_count);
void MyTableTest_get_refs(MyTableTest * table, u64 * keys, u64 ** indexes, u64 count);
void MyTableTest_clear(MyTableTest * table);
void MyTableTest_unset(MyTableTest * table, u64 key);
bool MyTableTest_try_get(MyTableTest * table, u64 * index, f32 * x, f32 * y);
void MyTableTest_print(MyTableTest * table);

