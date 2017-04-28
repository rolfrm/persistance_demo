typedef struct{
  mem_area * area;
  mem_area * free_indexes;
  u32 element_size;
}index_table;

typedef struct{
  u32 index;
  u32 count;

}index_table_sequence;

index_table * index_table_create(const char * name, u32 element_size);
u32 index_table_alloc(index_table * table);
index_table_sequence index_table_alloc_sequence(index_table * table, u32 count);
void * index_table_lookup_sequence(index_table * table, index_table_sequence seq);
void index_table_remove_sequence(index_table * table, index_table_sequence * seq);
void index_table_resize_sequence(index_table * table, index_table_sequence * seq, u32 new_count);
void * index_table_lookup(index_table * table, u32 index);
void index_table_remove(index_table * table, u32 index);
u32 index_table_count(index_table * table);
void * index_table_all(index_table * table, u64 * cnt);
void index_table_clear(index_table * table);
bool index_table_contains(index_table * table, u32 index);
void index_table_optimize(index_table * table);
void index_table_destroy(index_table ** table);
