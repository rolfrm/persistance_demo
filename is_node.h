typedef struct _is_node{
  char ** column_names;
  char ** column_types;
  u64 count;
  const bool is_multi_table;
  const int column_count;
  int (*cmp) (const u32 * k1, const u32 * k2);
  const u64 sizes[1];

  u32 * index;
  mem_area * index_area;
}is_node;

is_node * is_node_create(const char * optional_name);
void is_node_set(is_node * table, u32 index);
void is_node_insert(is_node * table, u32 * index, u64 count);
void is_node_lookup(is_node * table, u32 * keys, u64 * out_indexes, u64 count);
void is_node_remove(is_node * table, u32 * keys, u64 key_count);
void is_node_get_refs(is_node * table, u32 * keys, u64 ** indexes, u64 count);
void is_node_clear(is_node * table);
void is_node_unset(is_node * table, u32 key);
bool is_node_try_get(is_node * table, u32 * index);
void is_node_print(is_node * table);

