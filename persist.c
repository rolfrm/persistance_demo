#include <iron/full.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct{
  void * ptr;
  size_t size;
  char * name;
  int fd;
}persisted_mem_area;

persisted_mem_area * mem_areas = NULL;
u64 mem_area_cnt = 0;

u64 file_size(const char * path){
  int fd = open(path, O_RDWR | O_CREAT, 0666);
  u64 end = lseek(fd, 0, SEEK_END);
  close(fd);
  return end;
}

persisted_mem_area * get_mem_area_by_name(const char * name){
  for(u64 i = 0; i < mem_area_cnt; i++){
    if(strcmp(mem_areas[i].name, name) == 0)
      return mem_areas + i;
  }
  return NULL;
}

persisted_mem_area * get_mem_area_by_ptr(const void * ptr){
  for(u64 i = 0; i < mem_area_cnt; i++){
    if(mem_areas[i].ptr == ptr)
      return mem_areas + i;
  }
  return NULL;
}

void * persist_alloc(const char * name, size_t min_size){
  if(min_size == 0)
    min_size = 1;
  {
    persisted_mem_area * area = get_mem_area_by_name(name);
    if(area != NULL)
      return area->ptr;
  }
  
  const char * data_directory = "data";
  
  char path[100];
  sprintf(path, "%s/%s",data_directory, name);
  int fd = open(path, O_RDWR | O_CREAT, 0666);
  ASSERT(fd >= 0);
  u64 size = lseek(fd, 0, SEEK_END);
  if(size < min_size){
    lseek(fd, 0, SEEK_SET);
    ASSERT(0 == ftruncate(fd, min_size));
    size = lseek(fd, 0, SEEK_END);
  }
  logd("new size: %i\n", size);
  lseek(fd, 0, SEEK_SET);
  void * ptr = mmap(NULL, size, PROT_READ | PROT_WRITE , MAP_SHARED, fd, 0);
  ASSERT(ptr != NULL);
  persisted_mem_area mema = {.ptr = ptr, .size = size, .name = fmtstr("%s", name), .fd = fd};
  list_push2(mem_areas, mem_area_cnt, mema);
  return ptr;
}

void * persist_realloc(void * ptr, u64 size){
  persisted_mem_area * area = get_mem_area_by_ptr(ptr);
  ASSERT(area != NULL);
  if(area->size == size)
    return area->ptr;
  area->ptr = mremap(area->ptr, area->size, size, MREMAP_MAYMOVE);
  ASSERT(0 == ftruncate(area->fd, size));
  area->size = size;
  return area->ptr;
}

u64 persist_size(void * ptr){
  persisted_mem_area * area = get_mem_area_by_ptr(ptr);
  return area->size;
}
