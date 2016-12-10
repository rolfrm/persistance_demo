typedef struct{
  mem_area * ptr;
  mem_area * free_indexes;
  u32 element_size;
}index_table;

index_table * index_table_create(const char * name, u32 element_size);
u32 index_table_alloc(index_table * table);
void * index_table_lookup(index_table * table, u32 index);
void index_table_remove(index_table * table, u32 index);
