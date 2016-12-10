typedef struct{
  mem_area * ptr;
  mem_area * free_indexes;
  u32 element_size;
}index_table;

index_table * create_index_table(const char * name, u32 element_size);
u32 alloc_index_table(index_table * table);
void * lookup_index_table(index_table * table, u32 index);
