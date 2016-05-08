void * persist_realloc(void * ptr, u64 size);
void * persist_alloc(const char * name, size_t min_size);
u64 persist_size(void * ptr);
