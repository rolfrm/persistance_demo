typedef struct _color_alpha{
  char ** column_names;
  char ** column_types;
  u64 count;
  const bool is_multi_table;
  const int column_count;
  int (*cmp) (const u64 * k1, const u64 * k2);
  const u64 sizes[2];

  u64 * key;
  vec4 * color;
  mem_area * key_area;
  mem_area * color_area;
}color_alpha;

color_alpha * color_alpha_create(const char * optional_name);
void color_alpha_set(color_alpha * table, u64 key, vec4 color);
void color_alpha_insert(color_alpha * table, u64 * key, vec4 * color, u64 count);
void color_alpha_lookup(color_alpha * table, u64 * keys, u64 * out_indexes, u64 count);
void color_alpha_remove(color_alpha * table, u64 * keys, u64 key_count);
void color_alpha_get_refs(color_alpha * table, u64 * keys, u64 ** indexes, u64 count);
void color_alpha_clear(color_alpha * table);
void color_alpha_unset(color_alpha * table, u64 key);
bool color_alpha_try_get(color_alpha * table, u64 * key, vec4 * color);
void color_alpha_print(color_alpha * table);

