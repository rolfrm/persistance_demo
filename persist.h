
typedef struct{
  void * ptr;
  size_t size;
  char * name;
  int fd;
  bool only_32bit;
  bool is_persisted;
}persisted_mem_area;

typedef persisted_mem_area mem_area;
void mem_area_set_data_directory(char * data_dir);
persisted_mem_area * create_mem_area(const char * name);
persisted_mem_area * create_mem_area2(const char * name, bool only32bit);
persisted_mem_area * create_non_persisted_mem_area();
#define mem_area_create(X) create_mem_area(X)
#define mem_area_create_non_persisted() create_non_persisted_mem_area()


void mem_area_realloc(persisted_mem_area * area, u64 size);
void mem_area_free(persisted_mem_area * area);
// Reallocates a already persisted file.
void * persist_realloc(void * ptr, u64 size);
void * persist_realloc2(void * ptr, u64 size, u64 * out_size);

// Creates a new or opens a persisted file by name "name". If it does not exist it creates a blank file of size min_size.
void * persist_alloc(const char * name, size_t min_size);

// Creates a new or opens a persisted file by name "name". If it does not exist it creates a blank file of size min_size.
void * persist_alloc2(const char * name, size_t min_size, size_t * out_size);

// Gets the size of a persistent file.
u64 persist_size(void * ptr);
