typedef struct{
  mem_area * key_area;
  mem_area * value_area;
  size_t key_size, value_size;
  int (*cmp)(const void * k1, const void * k2);
  bool is_multi_table;
}sorttable;

bool sorttable_keys_sorted(sorttable * table, void * keys, u64 cnt);
u64 sorttable_find(sorttable * table, void * key);
void sorttable_finds(sorttable * table, void * keys, u64 * indexes, u64 cnt);
sorttable create_sorttable(size_t key_size, size_t value_size, const char * name);
u64 sorttable_insert_key(sorttable * table, void * key);
void sorttable_insert(sorttable * table, void * key, void * value);
void sorttable_insert_keys(sorttable * table, void * keys, u64 * out_indexes, u64 cnt);
void sorttable_inserts(sorttable * table, void * keys, void * values, size_t cnt);
void sorttable_removes(sorttable * table, void * keys, size_t cnt);

#define CREATE_TABLE_DECL2(Name, KeyType, ValueType)\
  sorttable * Name ## _initialize();		    \
  void insert_ ## Name(KeyType * keys, ValueType * values, size_t cnt);\
  void lookup_ ##Name(KeyType * keys, ValueType * out_values, size_t cnt); \
  void remove_ ## Name (KeyType * keys, size_t cnt);			\
  void get_refs_ ## Name(KeyType * keys, ValueType ** values, size_t cnt);\
  ValueType get_ ## Name(KeyType key);					\
  void set_ ## Name(KeyType key, ValueType val);			\
  u64 iter_all_ ## Name(KeyType * keys, ValueType * values, size_t _cnt, u64 * idx); \
  bool try_get_ ## Name(KeyType key, ValueType * value);		\
  void clear ## Name();\
  void unset_ ## Name(KeyType key);

#define _CREATE_TABLE2(Name, KeyType, ValueType, IS_MULTI_TABLE)	\
  sorttable * Name ## _initialize(){		\
    static bool initialized = false;		\
    static sorttable table;			\
    if(!initialized){				\
      initialized = true;			\
      table = create_sorttable(sizeof(KeyType), sizeof(ValueType), #Name); \
      table.is_multi_table = IS_MULTI_TABLE;				\
    }									\
    return &table;							\
  }									\
  void insert_ ## Name(KeyType * keys, ValueType * values, size_t cnt){	\
    sorttable_inserts(Name ## _initialize(), keys, values, cnt);	\
  }									\
  void lookup_ ##Name(KeyType * keys, ValueType * out_values, size_t cnt){ \
    u64 indexes[cnt];						       \
    sorttable_finds(Name ## _initialize(), keys, indexes, cnt);	       \
    ValueType * src = (Name ## _initialize())->value_area->ptr;	       \
    for(u64 i = 0; i < cnt; i++) if(indexes[i] != 0) out_values[i] = src[indexes[i]]; \
  }\
  void remove_ ## Name (KeyType * keys, size_t cnt){\
    sorttable_removes(Name ## _initialize(), keys, cnt);\
  }\
  void get_refs_ ## Name(KeyType * keys, ValueType ** values, size_t cnt){\
    u64 indexes[cnt];							\
    sorttable_finds(Name ## _initialize(), keys, indexes, cnt);		\
    ValueType * src = (Name ## _initialize())->value_area->ptr;		\
    for(u64 i = 0; i < cnt; i++) values[i] = (indexes[i] == 0 ? NULL : src + indexes[i]); \
  }									\
  ValueType get_ ## Name(KeyType key){					\
    ValueType value;							\
    memset(&value, 0, sizeof(value));					\
    lookup_ ## Name(&key, &value, 1);						\
    return value;							\
  }									\
  void set_ ## Name(KeyType key, ValueType val){			\
    insert_ ## Name(&key, &val, 1);					\
  }									\
  u64 iter_all_ ## Name(KeyType * keys, ValueType * values, size_t _cnt, u64 * idx){ \
    sorttable * area = Name ## _initialize();				\
    u64 i;								\
    u64 cnt = area->key_area->size / sizeof(KeyType);			\
    KeyType * k = area->key_area->ptr;					\
    ValueType * v = area->value_area->ptr;				\
    for(i = 0; i < _cnt && *idx < cnt; (*idx)++) {			\
      keys[i] = k[*idx];						\
      values[i] = v[*idx];						\
    }									\
    return i;								\
  }									\
  bool try_get_ ## Name(KeyType key, ValueType * value){\
    ValueType * v;					\
    get_refs_ ## Name(&key, &v, 1);			\
    if(v == NULL) return false;				\
    *value = *v;					\
    return true;					\
  }   \
    void clear ## Name(){\
    sorttable * area = Name ## _initialize(); \
    mem_area_realloc(area->key_area, sizeof(KeyType));\
    mem_area_realloc(area->key_area, sizeof(ValueType));\
    }\
    void unset_ ## Name(KeyType key){		\
    remove_ ## Name(&key, 1);\
    }
			 
#define CREATE_TABLE2(Name, KeyType, ValueType)\
  _CREATE_TABLE2(Name, KeyType, ValueType, false)

#define CREATE_MULTI_TABLE2(Name, KeyType, ValueType)\
  _CREATE_TABLE2(Name, KeyType, ValueType, true)
