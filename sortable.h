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
sorttable create_sorttable2(size_t key_size, size_t value_size, const char * name, bool only_32bit);
u64 sorttable_insert_key(sorttable * table, void * key);
void sorttable_insert(sorttable * table, void * key, void * value);
void sorttable_insert_keys(sorttable * table, void * keys, u64 * out_indexes, u64 cnt);
void sorttable_inserts(sorttable * table, void * keys, void * values, size_t cnt);
void sorttable_removes(sorttable * table, void * keys, size_t cnt);
void sorttable_remove_indexes(sorttable * table, u64 * indexes, size_t index_cnt);
size_t sorttable_iter(sorttable * table, void * keys, size_t keycnt, void * out_keys, u64 * indexes, size_t cnt, size_t * idx);
void sorttable_clear(sorttable * table);
void sorttable_destroy(sorttable * table);

#define CREATE_TABLE_DECL2(Name, KeyType, ValueType)	\
  sorttable * Name ## _initialize();		    \
  void insert_ ## Name(KeyType * keys, ValueType * values, size_t cnt);\
  void lookup_ ##Name(KeyType * keys, ValueType * out_values, size_t cnt); \
  void remove_ ## Name (KeyType * keys, size_t cnt);			\
  void get_refs_ ## Name(KeyType * keys, ValueType ** values, size_t cnt); \
  ValueType * get_ref_ ## Name(KeyType key);				\
  void get_refs2_ ## Name(KeyType * keys, ValueType ** values, KeyType ** keyref, size_t cnt); \
  ValueType get_ ## Name(KeyType key);					\
  void set_ ## Name(KeyType key, ValueType val);			\
  u64 iter_all_ ## Name(KeyType * keys, ValueType * values, size_t _cnt, u64 * idx); \
  u64 iter_ ## Name(KeyType * keys, size_t key_cnt, void * out_keys, u64 * out_indexes, size_t idx_cnt, size_t * idx); \
  u64 iter2_ ## Name(KeyType key, ValueType * values, u64 cnt, size_t * idx); \
  bool try_get_ ## Name(KeyType key, ValueType * value);		\
  void clear_ ## Name();							\
  void unset_ ## Name(KeyType key);\
  KeyType * get_keys_ ## Name();\
  ValueType * get_values_ ## Name();\
  u64 get_count_ ## Name();\
  ValueType * ref_at_ ## Name(u64 index);\
  void remove_at_ ## Name(u64 * index, size_t cnt); \
  typedef struct { sorttable * ptr; } Name ## _table; \
  Name ## _table * Name ## _table_create(const char * persisted_name);	\
  void Name ## _table_destroy(Name ## _table ** table);			\
  void Name ## _insert(Name ## _table * table, KeyType * keys, ValueType * values, size_t cnt); \
  void Name ## _lookup (Name ## _table * table, KeyType * keys, ValueType * out_values, size_t cnt); \
  void Name ## _remove (Name ## _table * table,KeyType * keys, size_t cnt);			\
  void Name ## _get_refs (Name ## _table * table,KeyType * keys, ValueType ** values, size_t cnt); \
  ValueType * Name ## _get_ref (Name ## _table * table,KeyType key);				\
  void Name ## _get_refs2(Name ## _table * table,KeyType * keys, ValueType ** values, KeyType ** keyref, size_t cnt); \
  ValueType Name ## _get(Name ## _table * table,KeyType key);					\
  void Name ## _set(Name ## _table * table,KeyType key, ValueType val);			\
  u64 Name ## _iter_all(Name ## _table * table,KeyType * keys, ValueType * values, size_t _cnt, u64 * idx); \
  u64 Name ## _iter(Name ## _table * table,KeyType * keys, size_t key_cnt, void * out_keys, u64 * out_indexes, size_t idx_cnt, size_t * idx); \
  u64 Name ## _iter2 (Name ## _table * table,KeyType key, ValueType * values, u64 cnt, size_t * idx); \
  bool Name ## _try_get(Name ## _table * table,KeyType key, ValueType * value);		\
  void Name ## _clear(Name ## _table * table);							\
  void Name ## _unset(Name ## _table * table, KeyType key);\
  ValueType * Name ## _ref_at(Name ## _table * table, u64 index);\
  void Name ## _remove_at(Name ## _table * table, u64 * index, size_t cnt);\
  u64 Name ## _count(Name ## _table * table);\
  u64 Name ## _get_count(Name ## _table * table);		\
  KeyType * Name ## _get_keys(Name ## _table * table);\
  ValueType * Name ## _get_values(Name ## _table * table);	\


#define _CREATE_TABLE2(Name, KeyType, ValueType, IS_MULTI_TABLE, IS_PERSISTED)	\
  sorttable * Name ## _initialize(){		\
    static bool initialized = false;		\
    static sorttable table;			\
    if(!initialized){				\
      initialized = true;			\
      table = create_sorttable(sizeof(KeyType), sizeof(ValueType), IS_PERSISTED ? #Name : NULL); \
      table.is_multi_table = IS_MULTI_TABLE;				\
    }									\
    return &table;							\
  }									\
  Name ## _table * Name ## _table_create(const char * persisted_path){		\
    sorttable table;							\
    table = create_sorttable(sizeof(KeyType), sizeof(ValueType), persisted_path); \
    table.is_multi_table = IS_MULTI_TABLE;				\
    Name ## _table _table = {.ptr = IRON_CLONE(table)};			\
    return IRON_CLONE(_table);						\
  }									\
  void Name ## _table_destroy(Name ## _table ** table){			\
    sorttable_destroy(table[0]->ptr);dealloc(table[0]->ptr);*table = NULL; \
  }									\
									\
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
  ValueType * get_ref_ ## Name(KeyType key) {				\
    ValueType * r = NULL;						\
    get_refs_ ## Name(&key, &r, 1);						\
    return r;								\
  }									\
  void get_refs2_ ## Name(KeyType * keys, ValueType ** values, KeyType ** keyref, size_t cnt){ \
    u64 indexes[cnt];							\
    sorttable_finds(Name ## _initialize(), keys, indexes, cnt);		\
    sorttable * table = (Name ## _initialize());			\
    ValueType * src = table->value_area->ptr;				\
    KeyType * keysrc = table->key_area->ptr;				\
    for(u64 i = 0; i < cnt; i++)					\
      if(indexes[i] != 0){						\
	values[i] = src + indexes[i];					\
	keyref[i] = keysrc + indexes[i];				\
      }else{values[i] = NULL; keyref[i] = NULL;}			\
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
    if(*idx == 0) *idx = 1;						\
    KeyType * k = area->key_area->ptr;					\
    ValueType * v = area->value_area->ptr;				\
    for(i = 0; i < _cnt && *idx < cnt; (*idx)++,i++) {			\
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
    void clear_ ## Name(){\
    sorttable * area = Name ## _initialize(); \
    sorttable_clear(area);		      \
    }						\
    void unset_ ## Name(KeyType key){		\
    remove_ ## Name(&key, 1);\
    }			     \
    KeyType * get_keys_ ## Name(){					\
      sorttable * area = Name ## _initialize();				\
      return ((KeyType *)area->key_area->ptr) + 1;				\
    }									\
    ValueType * get_values_ ## Name(){					\
      sorttable * area = Name ## _initialize();				\
      return ((ValueType *)area->value_area->ptr) + 1;			\
    }									\
    u64 get_count_ ## Name(){						\
       u64 c = (Name ## _initialize())->key_area->size / sizeof(KeyType);\
       return c == 0 ? 0 : (c - 1);					\
    }									\
    u64 iter_ ## Name(KeyType * keys, size_t key_cnt, void * out_keys, u64 * out_indexes, size_t idx_cnt, size_t * idx){ \
      sorttable * table = Name ## _initialize();			\
      return sorttable_iter(table, keys, key_cnt, out_keys, out_indexes, idx_cnt, idx); \
    }									\
    u64 iter2_ ## Name(KeyType key, ValueType * values, u64 cnt, size_t * idx){ \
      sorttable * table = Name ## _initialize();			\
      ValueType * p = table->value_area->ptr;				\
      u64 indexes[cnt];							\
      u64 cnt2 = sorttable_iter(table, &key, 1, NULL, indexes, cnt, idx);\
      for(u64 i=0;i < cnt2; i++) values[i] = p[indexes[i]];		\
      return cnt2;							\
    }									\
    ValueType * ref_at_ ## Name(u64 index) {				\
      sorttable * table = Name ## _initialize();			\
      ASSERT(index < (table->value_area->size / table->value_size));	\
      return table->value_area->ptr + index * table->value_size;	\
    }									\
    void remove_at_ ## Name(u64 * index, size_t cnt){			\
      sorttable_remove_indexes(Name ## _initialize(), index, cnt);	\
    }									\
    void Name ## _insert(Name ## _table * table, KeyType * keys, ValueType * values, size_t cnt){ \
      sorttable_inserts(table->ptr, keys, values, cnt);			\
    }									\
    void Name ## _lookup (Name ## _table * table, KeyType * keys, ValueType * out_values, size_t cnt){ \
    u64 indexes[cnt];						       \
    sorttable_finds(table->ptr, keys, indexes, cnt);	       \
    ValueType * src = table->ptr->value_area->ptr;	       \
    for(u64 i = 0; i < cnt; i++) if(indexes[i] != 0) out_values[i] = src[indexes[i]]; \
  }\
    void Name ## _remove  (Name ## _table * table, KeyType * keys, size_t cnt){ \
    sorttable_removes(table->ptr, keys, cnt);\
  }\
  void Name ## _get_refs(Name ## _table * table, KeyType * keys, ValueType ** values, size_t cnt){\
    u64 indexes[cnt];							\
    sorttable_finds(table->ptr, keys, indexes, cnt);		\
    ValueType * src = table->ptr->value_area->ptr;		\
    for(u64 i = 0; i < cnt; i++) values[i] = (indexes[i] == 0 ? NULL : src + indexes[i]); \
  }									\
  ValueType * Name ## get_ref(Name ## _table * table, KeyType key) {				\
    ValueType * r = NULL;						\
    Name ## _get_refs(table, &key, &r, 1);				\
    return r;								\
  }									\
  void Name ## _get_refs2(Name ## _table * _table, KeyType * keys, ValueType ** values, KeyType ** keyref, size_t cnt){ \
    u64 indexes[cnt];							\
    sorttable_finds(_table->ptr, keys, indexes, cnt);		\
    sorttable * table = _table->ptr;			\
    ValueType * src = table->value_area->ptr;				\
    KeyType * keysrc = table->key_area->ptr;				\
    for(u64 i = 0; i < cnt; i++)					\
      if(indexes[i] != 0){						\
	values[i] = src + indexes[i];					\
	keyref[i] = keysrc + indexes[i];				\
      }else{values[i] = NULL; keyref[i] = NULL;}			\
  }									\
  ValueType Name ## _get(Name ## _table * table, KeyType key){					\
    ValueType value;							\
    memset(&value, 0, sizeof(value));					\
    Name ## _lookup(table, &key, &value, 1);				\
    return value;							\
  }									\
  void Name ## _set(Name ## _table * table, KeyType key, ValueType val){			\
    Name ## _insert(table, &key, &val, 1);				\
  }									\
  u64 Name ## _iter_all(Name ## _table * table, KeyType * keys, ValueType * values, size_t _cnt, u64 * idx){ \
    sorttable * area = table->ptr;					\
    u64 i;								\
    u64 cnt = area->key_area->size / sizeof(KeyType);			\
    if(*idx == 0) *idx = 1;						\
    KeyType * k = area->key_area->ptr;					\
    ValueType * v = area->value_area->ptr;				\
    for(i = 0; i < _cnt && *idx < cnt; (*idx)++,i++) {			\
      keys[i] = k[*idx];						\
      values[i] = v[*idx];						\
    }									\
    return i;								\
  }									\
  bool Name ##_try_get(Name ## _table * table, KeyType key, ValueType * value){\
    ValueType * v;							\
    Name ## _get_refs(table, &key, &v, 1);				\
    if(v == NULL) return false;						\
    *value = *v;							\
    return true;					\
  }   \
    void Name ## _clear(Name ## _table * table){\
    sorttable_clear(table->ptr);		      \
    }						\
    void Name ## _unset(Name ## _table * table, KeyType key){		\
      Name ## _remove(table, &key, 1);				\
    }			     \
    u64 Name ## _iter(Name ## _table * table, KeyType * keys, size_t key_cnt, void * out_keys, u64 * out_indexes, size_t idx_cnt, size_t * idx){ \
      return sorttable_iter(table->ptr, keys, key_cnt, out_keys, out_indexes, idx_cnt, idx); \
    }									\
    u64 Name ## _iter2(Name ## _table * _table, KeyType key, ValueType * values, u64 cnt, size_t * idx){ \
      sorttable * table = _table->ptr;			\
      ValueType * p = table->value_area->ptr;				\
      u64 indexes[cnt];							\
      u64 cnt2 = sorttable_iter(table, &key, 1, NULL, indexes, cnt, idx);\
      for(u64 i=0;i < cnt2; i++) values[i] = p[indexes[i]];		\
      return cnt2;							\
    }									\
    ValueType * Name ## _ref_at (Name ## _table * table,u64 index) {				\
      									\
      ASSERT(index < (table->ptr->value_area->size / table->ptr->value_size)); \
      return table->ptr->value_area->ptr + index * table->ptr->value_size;	\
    }									\
    void Name ## _remove_at(Name ## _table * table, u64 * index, size_t cnt){			\
      sorttable_remove_indexes(table->ptr, index, cnt);	\
    }\
    u64 Name ## _count(Name ## _table * table){\
      return table->ptr->key_area->size / sizeof(KeyType) - 1; \
    }\
    u64 Name ## _get_count(Name ## _table * table){\
      return table->ptr->key_area->size / sizeof(KeyType) - 1; \
    }\
    KeyType * Name ## _get_keys(Name ## _table * table){\
      return ((KeyType *) table->ptr->key_area->ptr) + 1;\
    }\
    ValueType * Name ## _get_values(Name ## _table * table){\
      return ((ValueType *) table->ptr->value_area->ptr) + 1;\
    }

			 
#define CREATE_TABLE2(Name, KeyType, ValueType)\
  _CREATE_TABLE2(Name, KeyType, ValueType, false, true)

#define CREATE_TABLE_NP(Name, KeyType, ValueType)\
  _CREATE_TABLE2(Name, KeyType, ValueType, false, false)

#define CREATE_MULTI_TABLE2(Name, KeyType, ValueType)\
  _CREATE_TABLE2(Name, KeyType, ValueType, true, true)
