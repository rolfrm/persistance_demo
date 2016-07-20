typedef struct{
  u64 base_class;
  u64 class;
}subclass;

typedef struct{
  u64 id;
}class;

typedef void (* method)(u64 control, ...);

subclass * define_subclass(u64 class, u64 base_class);
u64 get_baseclass(u64 class, u64 * index);
void define_method(u64 class_id, u64 method_id, method handler);
method get_method(u64 class_id, u64 method_id);

class * new_class(u64 id);
method get_command_handler(u64 control_id, u64 command_id);

method * get_command_handler2(u64 control_id, u64 command_id, bool create);
void attach_handler(u64 control_id, u64 command_id, void * handler);

void * find_item(const char * table, u64 itemid, u64 size, bool create);

//persisted_mem_area * create_mem_area(const char * name);
//void mem_area_realloc(persisted_mem_area * area, u64 size)

#define CREATE_TABLE_DECL(Name, KeyType, ValueType) \
  void set_ ## Name(KeyType key, ValueType value);  \
  ValueType get_ ## Name (KeyType key);	 

#define CREATE_TABLE(Name, KeyType, ValueType)\
  persisted_mem_area * Name ## Initialize(){   \
    static persisted_mem_area * mem = NULL;   \
    if(mem == NULL){			      \
      mem = create_mem_area(#Name);      \
    }					      \
    return mem;				      \
  }					      \
							\
  void set_ ## Name(KeyType key, ValueType value){	\
								\
    		\
    persisted_mem_area * mem = Name ## Initialize();		\
								\
								\
    struct {							\
      KeyType key;						\
      ValueType value;						\
    } * data = mem->ptr;					\
    size_t item_size = sizeof(*data);				\
    u64 cnt = mem->size / item_size;				\
    for(size_t i = 0; i < cnt; i++){				\
      if(data[i].key == key){					\
	data[i].value = value;					\
	return;							\
      }								\
    }								\
    mem_area_realloc(mem, mem->size + item_size);		\
    data = mem->ptr;						\
    data[cnt].key = key;					\
    data[cnt].value = value;					\
  }								\
								\
  ValueType get_ ## Name (KeyType key){			\
    size_t item_size = sizeof(key) + sizeof(ValueType);		\
    persisted_mem_area * mem = Name ## Initialize();		\
								\
    u64 cnt = mem->size / item_size;				\
    struct {							\
      KeyType key;							\
      ValueType value;							\
    } * data = mem->ptr;					\
								\
    for(size_t i = 0; i < cnt; i++){				\
      if(data[i].key == key)					\
	return data[i].value;					\
								\
    }								\
								\
    struct {ValueType _default;} _default = {};				\
    return _default._default;						\
    								\
  }								\
								\
void clear_ ## Name(){						\
  persisted_mem_area * mem = Name ## Initialize();		\
  mem_area_realloc(mem, 1);					\
}


#define CREATE_MULTI_TABLE(Name, KeyType, ValueType)\
  persisted_mem_area * Name ## Initialize(){   \
    static persisted_mem_area * mem = NULL;   \
    if(mem == NULL){			      \
      mem = create_mem_area(#Name);      \
    }					      \
    return mem;				      \
  }					      \
							\
  void add_ ## Name(KeyType key, ValueType value){	\
								\
								\
    persisted_mem_area * mem = Name ## Initialize();		\
    struct {							\
      KeyType key;						\
      ValueType value;						\
    } * data = mem->ptr;					\
    size_t item_size = sizeof(*data);				\
    u64 cnt = mem->size / item_size;				\
    for(size_t i = 0; i < cnt; i++){				\
      if(data[i].key == 0){					\
	data[i].value = value;					\
	data[i].key = key;					\
	return;							\
      }								\
    }								\
    mem_area_realloc(mem, mem->size + item_size);		\
    data = mem->ptr;						\
    data[cnt].key = key;					\
    data[cnt].value = value;					\
  }								\
								\
  size_t get_ ## Name (KeyType key, ValueType * values, size_t values_cnt){	\
    size_t item_size = sizeof(key) + sizeof(ValueType);			\
    persisted_mem_area * mem = Name ## Initialize();			\
									\
    u64 cnt = mem->size / item_size;					\
    struct {								\
      KeyType key;							\
      ValueType value;							\
    } * data = mem->ptr;						\
    size_t index = 0;							\
    for(size_t i = 0; i < cnt; i++){					\
      if(index >= values_cnt) return values_cnt;			\
      if(data[i].key == key){						\
	if(values != NULL)						\
	  values[index] = data[i].value;			        \
	index++;							\
      }									\
    }									\
  									\
    return index;							\
									\
  }									\
  size_t iter_ ## Name (KeyType key, ValueType * values, size_t values_cnt, size_t * i){ \
    size_t item_size = sizeof(key) + sizeof(ValueType);			\
    persisted_mem_area * mem = Name ## Initialize();			\
									\
    u64 cnt = mem->size / item_size;					\
    struct {								\
      KeyType key;							\
      ValueType value;							\
    } * data = mem->ptr;						\
    size_t index = 0;							\
    for(; *i < cnt; (*i)++){						\
      if(index >= values_cnt) return values_cnt;			\
      if(data[*i].key == key){						\
	if(values != NULL)						\
	  values[index] = data[*i].value;	    		        \
	index++;							\
      }									\
    }									\
  									\
    return index;							\
									\
  }									\
  void clear_item_ ## Name(KeyType key){				\
    size_t item_size = sizeof(key) + sizeof(ValueType);			\
    persisted_mem_area * mem = Name ## Initialize();			\
    u64 cnt = mem->size / item_size;					\
    struct {								\
      KeyType key;							\
      ValueType value;							\
    } * data = mem->ptr;						\
    for(size_t i = 0; i < cnt; i++){					\
      if(data[i].key == key) data[i].key = 0;				\
    }									\
  }									\
  void clear_ ## Name(){						\
    persisted_mem_area * mem = Name ## Initialize();			\
    mem_area_realloc(mem, 1);						\
  }


typedef struct{
  char chunk[16];
}string_chunk;

#define CREATE_STRING_TABLE(Name, KeyType)				\
  CREATE_MULTI_TABLE(Name ## _chunk, KeyType, string_chunk);		\
									\
  void set_ ## Name(KeyType item, const char * name){			\
    u64 len = strlen(name) + 1;						\
    string_chunk chunks[1 + len / sizeof(string_chunk)];		\
    memcpy(chunks, name, len);						\
    for(u32 x = 0; x < array_count(chunks); x++){			\
      add_ ## Name ## _chunk(item, chunks[x]);				\
    }									\
  }									\
  									\
  u64 get_ ## Name(KeyType item, char * out, u64 buffer_size){		\
    if(buffer_size == 0) return 0;					\
    u64 l = get_##Name##_chunk(item, (void *)out, buffer_size / sizeof(string_chunk)); \
    return l * sizeof(string_chunk);					\
  }									\
  void remove_ ## Name(KeyType item){					\
    clear_item_ ## Name ## _chunk(item);				\
  }						

#define CREATE_STRING_TABLE_DECL(Name, KeyType)				\
  u64 get_ ## Name(KeyType item, char * out, u64 buffer_size);		\
  void set_ ## Name(KeyType item, const char * name);			\
  void remove_ ## Name(KeyType item);					
