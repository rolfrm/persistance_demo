
typedef struct{
  void * ptr;
  size_t size;
  char * name;
  int fd;
}persisted_mem_area;

typedef persisted_mem_area mem_area;

persisted_mem_area * create_mem_area(const char * name);
void mem_area_realloc(persisted_mem_area * area, u64 size);

// Reallocates a already persisted file.
void * persist_realloc(void * ptr, u64 size);
void * persist_realloc2(void * ptr, u64 size, u64 * out_size);

// Creates a new or opens a persisted file by name "name". If it does not exist it creates a blank file of size min_size.
void * persist_alloc(const char * name, size_t min_size);

// Creates a new or opens a persisted file by name "name". If it does not exist it creates a blank file of size min_size.
void * persist_alloc2(const char * name, size_t min_size, size_t * out_size);

// Gets the size of a persistent file.
u64 persist_size(void * ptr);
