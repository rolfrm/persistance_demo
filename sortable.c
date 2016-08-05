#include <iron/full.h>
#include "persist.h"
#include "sortable.h"

bool sorttable_keys_sorted(sorttable * table, void * keys, u64 cnt){
  for(u64 i = 0; i < cnt - 1; i++)
    if(table->cmp(keys + (i * table->key_size), keys + ((i + 1) * table->key_size)) >= 0)
      return false;
  return true;
}

u64 sorttable_find(sorttable * table, void * key){
  void * start = table->key_area->size + table->key_area->ptr;
  
  void * key_index = NULL;
  if(table->cmp(key, start) == 0)
    key_index = start;
  else
    key_index = bsearch(key, start, table->key_area->size / table->key_size, table->key_size, table->cmp);
  if(key_index == NULL) return 0;
  size_t offset = key_index - table->key_area->ptr;
  return (offset) / table->key_size;
}

void sorttable_finds(sorttable * table, void * keys, u64 * indexes, u64 cnt){
  void * start = table->key_area->ptr + table->key_size;
  void * end = table->key_area->ptr + table->key_area->size;
  for(u64 i = 0; i < cnt; i++){
    u64 size = end - start;
    void * key_index = NULL;
    void * key = keys + (i * table->key_size);
    if(table->cmp(key, start) == 0)
      key_index = start;
    else
      key_index =bsearch(key, start, size / table->key_size, table->key_size, table->cmp);
    
    if(key_index == 0){
      indexes[i] = 0;
    }else{
      indexes[i] = (key_index - table->key_area->ptr) / table->key_size;
      start = key_index + table->key_size;
    }
    
  }
}

static int keycmp(const u64 * k1,const  u64 * k2){
  if(*k1 > *k2)
    return 1;
  else if(*k1 == *k2)
    return 0;
  else return -1;
}

sorttable create_sorttable(size_t key_size, size_t value_size, const char * name){
  sorttable table;
  char pathbuf[100];
  sprintf(pathbuf, "table/%s.key", name);
  table.key_area = create_mem_area(pathbuf);
  if(table.key_area->size <= 8){
    mem_area_realloc(table.key_area, 8);
    memset(table.key_area->ptr, 0, 8);
  }
    
  sprintf(pathbuf, "table/%s.value", name);
  table.value_area = create_mem_area(pathbuf);
  table.key_size = key_size;
  table.value_size = value_size;
  table.cmp = (void *) keycmp;
  return table;
}

u64 sorttable_insert_key(sorttable * table, void * key){
  mem_area_realloc(table->key_area, table->key_area->size + table->key_size);
  mem_area_realloc(table->value_area, table->value_area->size + table->value_size);
  void * pt = table->key_area->ptr;
  void * end = pt + table->key_area->size - table->key_size;
  while(pt < end && table->cmp(pt, key) < 0)
    pt += table->key_size;

  u64 offset = (pt - table->key_area->ptr) / table->key_size;
  u64 newcnt = table->key_area->size / table->key_size;
  
  memmove(pt + table->key_size, pt , end - pt);
  memmove(pt, key, table->key_size);

  memmove(table->value_area->ptr + (offset + 1) * table->value_size,
	  table->value_area->ptr + offset * table->value_size,
	  (newcnt - offset  - 1) * table->value_size);
  
  memset(table->value_area->ptr + offset * table->value_size, 0, table->value_size);
  
  return (pt - table->key_area->ptr) / table->key_size;
}

void sorttable_insert(sorttable * table, void * key, void * value){
  u64 idx = sorttable_find(table, key);
  if(idx == 0)
    idx = sorttable_insert_key(table, key);
  memcpy(table->value_area->ptr + idx * table->value_size, value, table->value_size);
}

void sorttable_insert_keys(sorttable * table, void * keys, u64 * out_indexes, u64 cnt){
  mem_area_realloc(table->key_area, table->key_area->size + cnt * table->key_size);
  mem_area_realloc(table->value_area, table->value_area->size + cnt * table->value_size);
  void * pt = table->key_area->ptr;
  void * end = pt + table->key_area->size - table->key_size * cnt;
  for(u64 i = 0; i < cnt; i++){
    while(pt < end && table->cmp(pt, keys) < 0)
      pt += table->key_size;

    u64 offset = (pt - table->key_area->ptr) / table->key_size;
    u64 newcnt = table->key_area->size / table->key_size;
  
    memmove(pt + table->key_size, pt , end - pt);
    memmove(pt, keys, table->key_size);

    memmove(table->value_area->ptr + (offset + 1) * table->value_size,
	    table->value_area->ptr + offset * table->value_size,
	    (newcnt - offset  - 1) * table->value_size);
  
    memset(table->value_area->ptr + offset * table->value_size, 0, table->value_size);
  
    *out_indexes = (pt - table->key_area->ptr) / table->key_size;
    
    out_indexes += 1;
    keys += table->key_size;
    pt += table->key_size;
    end += table->key_size;
  }
}

void sorttable_inserts(sorttable * table, void * keys, void * values, size_t cnt){
  ASSERT(sorttable_keys_sorted(table, keys, cnt));
  u64 indexes[cnt];
  sorttable_finds(table, keys, indexes, cnt);
  u64 newcnt = 0;
  for(u64 i = 0; i < cnt; i++){
    if(indexes[i] == 0){
      newcnt += 1;
    }else{
      memcpy(table->value_area->ptr + table->value_size * indexes[i], values + table->value_size * i, table->value_size);
    }
  }
  u8 newkeys[newcnt * table->key_size];
  u8 newvalues[newcnt * table->value_size];
  u64 offset = 0;
  for(u64 i = 0; i < cnt; i++){
    if(indexes[i] == 0){
      memcpy(newvalues + table->value_size * offset, values + i * table->value_size, table->value_size);
      memcpy(newkeys + table->key_size * offset++, keys + i * table->key_size, table->key_size);
    }
  }
  sorttable_insert_keys(table, newkeys, indexes, newcnt);
  for(u64 i = 0; i < newcnt; i++){
    ASSERT(indexes[i]);
    memcpy(table->value_area->ptr + table->value_size * indexes[i], newvalues + table->value_size * i, table->value_size);
  }
}

void sorttable_removes(sorttable * table, void * keys, size_t cnt){
  ASSERT(sorttable_keys_sorted(table, keys, cnt));
  u64 indexes[cnt];
  void * pt = table->key_area->ptr;
  void * vpt = table->value_area->ptr;
  const size_t key_size = table->key_size;
  const size_t value_size = table->value_size;
  sorttable_finds(table, keys, indexes, cnt);
  u64 table_cnt = table->key_area->size / table->key_size;
  for(u64 _i = 0; _i < cnt; _i++){
    u64 i = cnt - _i - 1;
    if(indexes[i] == 0) continue;
    u64 index = indexes[i];
    memmove(pt + index * key_size, pt + (1 + index) * key_size, (table_cnt - index - 1) * key_size);
    memmove(vpt + index * value_size, vpt + (1 + index) * value_size, (table_cnt - index - 1) * value_size);
    table_cnt -= 1;
  }
  mem_area_realloc(table->key_area, table_cnt * key_size);
  mem_area_realloc(table->value_area, table_cnt * value_size);
}

CREATE_TABLE_DECL2(mtab2, u64, u64);
CREATE_TABLE2(mtab2, u64, u64);

void table2_test(){
  {
    u64 a = 5, b = 10;
    insert_mtab2(&a, &b, 1);
    u64 * c;
    get_refs_mtab2(&a, &c, 1);
    ASSERT(*c == 10);
    b = 0;
    lookup_mtab2(&a, &b, 1);
    ASSERT(b == 10);
    
    u64 keys[1000];
    u64 values[1000];
    for(u64 i = 100; i < 1100; i++){
      keys[i - 100] = i;
      values[i - 100] = i * 2;
    }
    insert_mtab2(keys, values, array_count(keys));
    
    for(u64 i = 100; i < 1100; i++){
      keys[i - 100] = i + 2000;
      values[i - 100] = i * 2;
    }
    insert_mtab2(keys, values, array_count(keys));
    
    for(u64 i = 100; i < 1100; i++){
      keys[i - 100] = i;
    }
    memset(values,0,sizeof(values));
    lookup_mtab2(keys, values, array_count(keys));
    for(u64 i = 100; i < 1100; i++){
      ASSERT(values[i - 100] == i * 2);
    }
    
    u64 keys2[2] = {500, 2600};
    u64 values2[2];
    lookup_mtab2(keys2, values2, 2);
    ASSERT(values2[0] == 1000);
    ASSERT(values2[1] == 1200);
    remove_mtab2(keys2, 2);
    memset(values2, 0, sizeof(values2));
    lookup_mtab2(keys2, values2, 2);
    ASSERT(values2[0] == 0 && values2[1] == 0);
  }
    sorttable tabl = create_sorttable(sizeof(u64), sizeof(vec4), "mytab");
  {
    u64 x = 5;
    vec4 v = vec4_new(1,1,1,1);
    sorttable_insert(&tabl, &x, &v);
  }
  {
    u64 x = 25;
    vec4 v = vec4_new(1,0,1,1);
    sorttable_insert(&tabl, &x, &v);
  }
  {
    u64 x = 15;
    vec4 v = vec4_new(1,1,0,1);
    sorttable_insert(&tabl, &x, &v);
  }
  {
    u64 x = 3;
    vec4 v = vec4_new(0,0,1,1);
    sorttable_insert(&tabl, &x, &v);
  }
  {
    u64 key = 5;
    u64 idx = sorttable_find(&tabl, &key);
    vec4 * ptrs = tabl.value_area->ptr;
    logd("INDEX: %i\n", idx); vec4_print(ptrs[idx]);logd("\n");
  }
  {
    u64 keys[] = {2, 5, 10, 15 ,25};
    ASSERT(sorttable_keys_sorted(&tabl, keys, array_count(keys)));
    u64 indexes[array_count(keys)];
    sorttable_finds(&tabl, keys, indexes, array_count(keys));
    logd("%i %i %i %i %i\n", indexes[0], indexes[1], indexes[2],indexes[3],indexes[4]);
  }

  {
    u64 keys2[] = {5, 8, 10, 15, 20, 23, 25};
    vec4 values[array_count(keys2)];
    for(u64 i = 0; i < array_count(keys2); i++)
      values[i] = vec4_new(i, i * 2, keys2[i], keys2[i]);
    sorttable_inserts(&tabl, keys2, values, array_count(keys2));
  }
  {
    u64 keys[] = {2, 5, 8, 10, 15, 20, 23 ,25};
    ASSERT(sorttable_keys_sorted(&tabl, keys, array_count(keys)));
    u64 indexes[array_count(keys)];
    sorttable_finds(&tabl, keys, indexes, array_count(keys));
    for(u64 i = 0; i < array_count(keys); i++)
      logd("%i ", indexes[i]);
    logd("\n");
  }

  {
    u64 keys[] = {2, 5, 8, 15, 23};
    sorttable_removes(&tabl, keys, array_count(keys));
  }
  logd("All keys:\n");
  u64 * keypt = tabl.key_area->ptr;
  vec4 * vecs = tabl.value_area->ptr;
  for(u64 i = 0; i < tabl.key_area->size / tabl.key_size; i++){
    logd("%i %i :", i, keypt[i]);vec4_print(vecs[i]); logd(" == "); vec4_print(vec4_new(i, i * 2,keypt[i], keypt[i])); logd("\n");
  }
}
