#include <iron/full.h>
#include "persist.h"
#include "abstract_sortable.h"

static int keycmp32(const u32 * k1,const  u32 * k2){
  if(*k1 > *k2)
    return 1;
  else if(*k1 == *k2)
    return 0;
  else return -1;
}

static int keycmp(const u64 * k1,const  u64 * k2){
  if(*k1 > *k2)
    return 1;
  else if(*k1 == *k2)
    return 0;
  else return -1;
}

static int keycmp128(const u128 * k1,const  u128 * k2){
  if(*k1 > *k2)
    return 1;
  else if(*k1 == *k2)
    return 0;
  else return -1;
}

static u64 * get_type_sizes(abstract_sorttable * table){
  return (u64 *) &table->tail;
}

static mem_area ** get_mem_areas(abstract_sorttable * table){
  return ((void *)  &table->tail) + sizeof(u64) * table->column_count + sizeof(void *) * table->column_count;
}

static void ** get_pointers(abstract_sorttable * table){
  return ((void *)  &table->tail) + sizeof(u64) * table->column_count;
}

static bool indexes_unique_and_sorted(u64 * indexes, u64 cnt){
  if(cnt == 0) return true;
  for(u64 i = 0; i < cnt - 1; i++)
    if(indexes[i] >= indexes[i + 1])
      return false;
  return true;
}

void abstract_sorttable_check_sanity(abstract_sorttable * table){
  mem_area ** areas = get_mem_areas(table);
  u64 * type_size = get_type_sizes(table);
  u64 cnt = 0;
  for(u32 i = 0; i < table->column_count; i++){
    if(cnt == 0)
      cnt = areas[i]->size / type_size[i];
    else
      ASSERT(cnt == areas[i]->size / type_size[i]);
  }
}

  
bool abstract_sorttable_keys_sorted(abstract_sorttable * table, void * keys, u64 cnt){
  u64 key_size = get_type_sizes(table)[0];
  if(cnt == 0) return true;
  if(table->is_multi_table){
    for(u64 i = 0; i < cnt - 1; i++)
      if(table->cmp(keys + (i * key_size), keys + (i + 1) * key_size) > 0)
	return false;
  }else{
    for(u64 i = 0; i < cnt - 1; i++)
      if(table->cmp(keys + (i * key_size), keys + (i + 1) * key_size) >= 0)
	return false;
  }
  return true;
}

void abstract_sorttable_init(abstract_sorttable * table,const char * table_name , u32 column_count, u32 * column_size, char ** column_name){
  char pathbuf[100];
  *((u32 *)(&table->column_count)) = column_count;
  mem_area ** mem_areas = get_mem_areas(table);
  void ** pointers = get_pointers(table);
  u64 * type_sizes = get_type_sizes(table);
  u64 key_size = column_size[0];
  if(key_size == sizeof(u128))
    table->cmp = (void *) keycmp128;
  else if(key_size == sizeof(u32)){

    table->cmp = (void *) keycmp32;
  }
  else
    table->cmp = (void *) keycmp;
  

  for(u32 i = 0; i < column_count; i++){
    type_sizes[i] = column_size[i];
    if(column_name[i] == NULL || table_name == NULL){
      mem_areas[i] = create_non_persisted_mem_area();
    }else{
      sprintf(pathbuf, "table2/%s.%s", table_name, column_name[i]);
      mem_areas[i] = create_mem_area(pathbuf);
    }
    if(mem_areas[i]->size < type_sizes[i]){
      mem_area_realloc(mem_areas[i], type_sizes[i]);
      memset(mem_areas[i]->ptr, 0, type_sizes[i]);
    }
    pointers[i] = mem_areas[i]->ptr;
  }
  
  abstract_sorttable_check_sanity(table);
}

void abstract_sorttable_finds(abstract_sorttable * table, void * keys, u64 * indexes, u64 cnt){
  ASSERT(abstract_sorttable_keys_sorted(table, keys, cnt));
  abstract_sorttable_check_sanity(table);
  memset(indexes, 0, cnt * sizeof(indexes[0]));
  u64 key_size = get_type_sizes(table)[0];
  mem_area * key_area = get_mem_areas(table)[0];
  
  if(key_area->size <= key_size)
    return;
  void * start = key_area->ptr + key_size;
  void * end = key_area->ptr + key_area->size;
  
  for(u64 i = 0; i < cnt; i++){

    //if(end < start) break;
    u64 size = end - start;
    void * key_index = NULL;
    void * key = keys + i * key_size;
    int startcmp = table->cmp(key, start);
    if(startcmp < 0) continue;
    if(startcmp == 0)
      key_index = start;
    else if(table->cmp(key, end - key_size) > 0){
      return;
    }else
      //key_index =memmem(start,size,key,table->key_size);
      key_index = bsearch(key, start, size / key_size, key_size, (void *)table->cmp);

    if(key_index == 0){
      indexes[i] = 0;
    }else{
      indexes[i] = (key_index - key_area->ptr) / key_size;
      start = key_index + key_size;
    }    
  }
}

void abstract_sorttable_insert_keys(abstract_sorttable * table, void * keys, u64 * out_indexes, u64 cnt){
  ASSERT(abstract_sorttable_keys_sorted(table, keys, cnt));
  abstract_sorttable_check_sanity(table);
  u64 * column_size = get_type_sizes(table);
  mem_area ** column_area = get_mem_areas(table);
  void ** pointers = get_pointers(table);
  mem_area * key_area = column_area[0];
  u64 key_size = column_size[0];
  u32 column_count = table->column_count;
  // make room for new data.
  for(u32 i = 0; i < column_count; i++){
    mem_area_realloc(column_area[i], column_area[i]->size + cnt * column_size[i]);
    pointers[i] = column_area[i]->ptr;
  }
  
  // skip key related things
  column_size += 1;
  column_area += 1;
  column_count -= 1;

  abstract_sorttable_check_sanity(table);  
  void * pt = key_area->ptr + key_size;
  void * end = key_area->ptr + key_area->size - key_size * cnt;

  void * vend[column_count];
  for(u32 i = 0; i < column_count; i++)
    vend[i] = column_area[i]->ptr + column_area[i]->size - column_size[i] * cnt;
  
  for(u64 i = 0; i < cnt; i++){
    while(pt < end && table->cmp(pt, keys) <= 0){
      pt += key_size;
    }
    u64 offset = (pt - key_area->ptr) / key_size;
    // move everything from keysize up
    memmove(pt + key_size, pt , end - pt); 
    memmove(pt, keys, key_size);

    for(u32 j = 0; j < column_count; j++){
      void * vpt = column_area[j]->ptr + offset * column_size[j];
      memmove(vpt + column_size[j], vpt, vend[j] - vpt);
      memset(vpt, 0, column_size[j]);
    }
  
    *out_indexes = (pt - key_area->ptr) / key_size;
    
    out_indexes += 1;
    keys += key_size;
    pt += key_size;
    end += key_size;
    for(u32 j = 0; j < table->column_count; j++){
      vend[j] += column_size[j];
    }
  }
  table->count = key_area->size / key_size - 1;
}

void abstract_sorttable_inserts(abstract_sorttable * table, void ** values, u64 cnt){
  abstract_sorttable_check_sanity(table);
  ASSERT(values[0] != NULL);
  void * keys = values[0];

  u64 * column_size = get_type_sizes(table);
  mem_area ** column_area = get_mem_areas(table);
  
  ASSERT(abstract_sorttable_keys_sorted(table, keys, cnt));
  u64 indexes[cnt];
  memset(indexes, 0, sizeof(indexes));
  u64 newcnt = 0;
  if(table->is_multi_table){
    // overwrite is never done for multi tables.
    newcnt = cnt;
  }else{
    abstract_sorttable_finds(table, keys, indexes, cnt);
    for(u64 i = 0; i < cnt ; i++){
      if(indexes[i] == 0)
	newcnt += 1;
    }
    if(newcnt != cnt){
      for(u32 j = 1; j < table->column_count; j++){
	mem_area * value_area = column_area[j];
	u32 size = column_size[j];
	for(u64 i = 0; i < cnt; i++){
	  if(indexes[i] != 0){
	    // overwrite existing values with new values
	    memcpy(value_area->ptr + size * indexes[i], values[j] + size * i, size);
	  }
	}
      }
    }
  }
  u64 indexes2[newcnt];
  memset(indexes2, 0, sizeof(indexes2));
  {
    u32 csize = column_size[0];
    u64 newsize = newcnt * csize;
    u8 * newvalues[newsize];
    u64 offset = 0;
    for(u64 i = 0; i < cnt; i++){
      if(indexes[i] == 0){
	indexes2[offset] = i;
	memcpy(newvalues + csize * offset, values[0] + i * csize, csize);
	offset += 1;
      }
    }
    memset(indexes, 0, sizeof(indexes));
  // make room and insert keys
    abstract_sorttable_insert_keys(table, newvalues, indexes, newcnt);
  }

  // Insert the new data
  for(u32 j = 1; j < table->column_count; j++){
    u64 csize = column_size[j];
    for(u64 i = 0; i < newcnt; i++){
      u64 idx = indexes2[i];
      ASSERT(indexes[i]);
      memcpy(column_area[j]->ptr + csize * indexes[i], values[j] + csize * idx, csize);
    }
  }
}

void abstract_sorttable_clear(abstract_sorttable * table){
  u64 * column_size = get_type_sizes(table);
  mem_area ** column_area = get_mem_areas(table);
  void ** pointers = get_pointers(table);
  
  for(u32 i = 0; i < table->column_count; i++){
    mem_area_realloc(column_area[i], column_size[i]);
    pointers[i] = column_area[i]->ptr;
  }
  table->count = 0;
}

void abstract_sorttable_remove_indexes(abstract_sorttable * table, u64 * indexes, size_t cnt){
  ASSERT(indexes_unique_and_sorted(indexes, cnt));

  u64 * column_size = get_type_sizes(table);
  mem_area ** column_area = get_mem_areas(table);
  void ** pointers = get_pointers(table);
  
  const u64 _table_cnt = column_area[0]->size / column_size[0];
  table->count = _table_cnt - cnt;
  for(u32 j = 0; j < table->column_count; j++){
    u64 table_cnt = _table_cnt;
    void * pt = column_area[j]->ptr;
    u64 size = column_size[j];
    for(u64 _i = 0; _i < cnt; _i++){
      u64 i = cnt - _i - 1;
      u64 index = indexes[i];
      memmove(pt + index * size, pt + (1 + index) * size, (table_cnt - index - 1) * size);
      table_cnt--;
    }
    mem_area_realloc(column_area[j], table_cnt * size);
    pointers[j] = column_area[j]->ptr;
  }
  abstract_sorttable_check_sanity(table);
}

void table_print_cell(void * ptr, const char * type);
void abstract_sorttable_print(abstract_sorttable * table){
  void ** pointers = get_pointers(table);
  u64 * sizes = get_type_sizes(table);
  for(u32 i = 0; i < table->column_count;i++)
    logd("%s ", table->column_types[i]);
  logd("\n");
  for(u32 i = 0; i < table->count; i++){
    for(u32 j = 0; j < table->column_count;j++){
      table_print_cell(pointers[j] + (1 + i) * sizes[j], table->column_types[j]);
      logd(" ");
    }
    logd("\n");
  }
}
