// Reallocates a already persisted file.
void * persist_realloc(void * ptr, u64 size);

// Creates a new or opens a persisted file by name "name". If it does not exist it creates a blank file of size min_size.
void * persist_alloc(const char * name, size_t min_size);
// Gets the size of a persistent file.
u64 persist_size(void * ptr);
