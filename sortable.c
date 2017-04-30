#include <iron/full.h>
#include "persist.h"
#include "sortable.h"

void sorttable_check_sanity(sorttable * table){
  ASSERT(table->key_area->size / table->key_size == table->value_area->size / table->value_size);
}

bool sorttable_keys_sorted(sorttable * table, void * keys, u64 cnt){
  if(cnt == 0) return true;
  if(table->is_multi_table){
    for(u64 i = 0; i < cnt - 1; i++)
      if(table->cmp(keys + (i * table->key_size), keys + ((i + 1) * table->key_size)) > 0)
	return false;
  }else{
    for(u64 i = 0; i < cnt - 1; i++)
      if(table->cmp(keys + (i * table->key_size), keys + ((i + 1) * table->key_size)) >= 0)
	return false;
  }
  return true;
}

static bool indexes_unique_and_sorted(u64 * indexes, u64 cnt){
  if(cnt == 0) return true;
  for(u64 i = 0; i < cnt - 1; i++)
    if(indexes[i] >= indexes[i + 1])
      return false;
  return true;
}

void sorttable_finds(sorttable * table, void * keys, u64 * indexes, u64 cnt){
  ASSERT(sorttable_keys_sorted(table, keys, cnt));
  sorttable_check_sanity(table);
  memset(indexes, 0, cnt * sizeof(u64));
  if(table->key_area->size <= table->key_size)
    return;
  void * start = table->key_area->ptr + table->key_size;
  void * end = table->key_area->ptr + table->key_area->size;// - table->key_size;

  for(u64 i = 0; i < cnt; i++){
    //if(end < start) break;
    u64 size = end - start;
    void * key_index = NULL;
    void * key = keys + i * table->key_size;
    int startcmp = table->cmp(key, start);
    if(startcmp < 0) continue;
    if(startcmp == 0)
      key_index = start;
    else if(table->cmp(key, end - table->key_size) > 0)
      return;
    else
      //key_index =memmem(start,size,key,table->key_size);
      key_index = bsearch(key, start, size / table->key_size, table->key_size, table->cmp);
    
    if(key_index == 0){
      indexes[i] = 0;
    }else{
      indexes[i] = (key_index - table->key_area->ptr) / table->key_size;
      start = key_index + table->key_size;
    }    
  }
}

u64 sorttable_find(sorttable * table, void * key){
  u64 idx = 0;
  sorttable_finds(table, key, &idx, 1);
  return idx;
}

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

sorttable create_sorttable(size_t key_size, size_t value_size, const char * name){
  return create_sorttable2(key_size, value_size, name, false);
}

sorttable create_sorttable2(size_t key_size, size_t value_size, const char * name, bool only_32bit){
  sorttable table = {};
  char pathbuf[100];
  if(name == NULL){
    table.key_area = create_non_persisted_mem_area();
    table.value_area = create_non_persisted_mem_area();
  }else{
    sprintf(pathbuf, "table/%s.key", name);
    table.key_area = create_mem_area(pathbuf);
    sprintf(pathbuf, "table/%s.value", name);
    table.value_area = create_mem_area2(pathbuf, only_32bit);
  }
  table.key_size = key_size;
  table.value_size = value_size;
  if(key_size == sizeof(u128))
    table.cmp = (void *) keycmp128;
  else if(key_size == sizeof(u32))
    table.cmp = (void *) keycmp32;
  else
    table.cmp = (void *) keycmp;
  
  if(table.key_area->size < key_size){
    mem_area_realloc(table.key_area, key_size);
    memset(table.key_area->ptr, 0, key_size);
    mem_area_realloc(table.value_area, value_size);
    memset(table.value_area->ptr, 0, value_size);
  }
  sorttable_check_sanity(&table);
  return table;
}

void sorttable_destroy(sorttable * table){
  mem_area_free(table->key_area);
  mem_area_free(table->value_area);
  table->key_area = NULL;
  table->value_area = NULL;
}

u64 sorttable_insert_key(sorttable * table, void * key){
  u64 idx = 0;
  sorttable_insert_keys(table, key, &idx, 1);
  return idx;
}

void sorttable_insert(sorttable * table, void * key, void * value){
  sorttable_inserts(table, key, value, 1);
}
void * bsearch_bigger(int (*cmp)(void*, void*), void * key, void * pt, void * end, size_t keysize){

  u64 a = 0;
  u64 cnt = ((u64)(end - pt)) / keysize;
  if(cnt == 0) return pt;
  if(cmp(pt + a, key) > 0)
    return pt;
  u64 b = cnt - 1;

  
  while(a != b){
    u32 c = (a + b) / 2;
    if(cmp(pt + c * keysize, key) > 0)
      b = c;
    else
      a = c + 1;
  }
  return pt + a * keysize;
}

void sorttable_insert_keys(sorttable * table, void * keys, u64 * out_indexes, u64 cnt){
  ASSERT(sorttable_keys_sorted(table, keys, cnt));
  sorttable_check_sanity(table);
  mem_area_realloc(table->key_area, table->key_area->size + cnt * table->key_size);
  mem_area_realloc(table->value_area, table->value_area->size + cnt * table->value_size);
  sorttable_check_sanity(table);  
  void * pt = table->key_area->ptr;
  void * end = pt + table->key_area->size - table->key_size * cnt;
  void * vend = table->value_area->ptr + table->value_area->size - table->value_size * cnt;
  int (*cmp)(const void*, const void*) = table->cmp;
  for(u64 i = 0; i < cnt; i++){
    pt = bsearch_bigger((void*)cmp, keys, pt, end, table->key_size);
    while(pt < end && cmp(pt, keys) <= 0)
      pt += table->key_size;
    u64 offset = (pt - table->key_area->ptr) / table->key_size;

    memmove(pt + table->key_size, pt , end - pt);
    memmove(pt, keys, table->key_size);

    void * vpt = table->value_area->ptr +  offset * table->value_size;
    
    memmove(vpt + table->value_size, vpt, vend - vpt);
    memset(vpt, 0, table->value_size);
  
    *out_indexes = (pt - table->key_area->ptr) / table->key_size;
    
    out_indexes += 1;
    keys += table->key_size;
    pt += table->key_size;
    end += table->key_size;
    vend += table->value_size;
  }
}

void sorttable_inserts(sorttable * table, void * keys, void * values, size_t cnt){
  sorttable_check_sanity(table);
  ASSERT(sorttable_keys_sorted(table, keys, cnt));
  u64 indexes[cnt];
  memset(indexes, 0, sizeof(indexes));
  u64 newcnt = 0;
  if(table->is_multi_table){
    newcnt = cnt;
  }else{
    sorttable_finds(table, keys, indexes, cnt);
    for(u64 i = 0; i < cnt; i++){
      if(indexes[i] == 0){
	newcnt += 1;
      }else{
	// overwrite existing values.
	memcpy(table->value_area->ptr + table->value_size * indexes[i], values + table->value_size * i, table->value_size);
      }
    }
  }
  u8 newkeys[newcnt * table->key_size];
  u8 newvalues[newcnt * table->value_size];
  u64 offset = 0;
  for(u64 i = 0; i < cnt; i++){
    if(indexes[i] == 0){
      memcpy(newvalues + table->value_size * offset, values + i * table->value_size, table->value_size);
      memcpy(newkeys + table->key_size * offset, keys + i * table->key_size, table->key_size);
      offset += 1;
    }
  }
  memset(indexes, 0, sizeof(indexes));
  if(offset == 0)
    return;
  sorttable_insert_keys(table, newkeys, indexes, newcnt);
  for(u64 i = 0; i < newcnt; i++){
    ASSERT(indexes[i]);
    memcpy(table->value_area->ptr + table->value_size * indexes[i], newvalues + table->value_size * i, table->value_size);
  }
}

void sorttable_remove_indexes(sorttable * table, u64 * indexes, size_t cnt){
  ASSERT(indexes_unique_and_sorted(indexes, cnt));
  u64 table_cnt = table->key_area->size / table->key_size;
  void * pt = table->key_area->ptr;
  void * vpt = table->value_area->ptr;
  const size_t key_size = table->key_size;
  const size_t value_size = table->value_size;
  
  for(u64 _i = 0; _i < cnt; _i++){
    u64 i = cnt - _i - 1;
    u64 index = indexes[i];
    memmove(pt + index * key_size, pt + (1 + index) * key_size, (table_cnt - index - 1) * key_size);
    memmove(vpt + index * value_size, vpt + (1 + index) * value_size, (table_cnt - index - 1) * value_size);
    indexes[i] = 0 ;
    table_cnt -= 1;
  }
  mem_area_realloc(table->key_area, table_cnt * key_size);
  mem_area_realloc(table->value_area, table_cnt * value_size);
  sorttable_check_sanity(table);
}

void sorttable_removes(sorttable * table, void * keys, size_t cnt){
  ASSERT(sorttable_keys_sorted(table, keys, cnt));
  u64 indexes[cnt];
  memset(indexes, 0, sizeof(indexes));
  sorttable_finds(table, keys, indexes, cnt);
  u64 j = 0;
  for(u64 i = 0; i < cnt; i++){
    if(indexes[i] == 0) continue;
    indexes[j++] = indexes[i];
  }
  sorttable_remove_indexes(table, indexes, j);
  memset(indexes, 0, sizeof(indexes));
  sorttable_finds(table, keys, indexes, cnt);
  for(u64 i = 0; i < cnt; i++){
    ASSERT(indexes[i] == 0);
  }
}

size_t sorttable_iter(sorttable * table, void * keys, size_t keycnt, void * out_keys, u64 * indexes, size_t cnt, size_t * idx){
  void * orig_out_keys = out_keys;
  u64 idx_replacement = 0;
  u64 orig_cnt = cnt;
  if(idx == NULL)
    idx = &idx_replacement;
  
  if(*idx == 0) *idx = 1;
  for(u64 i = 0; i < keycnt; i++){
    void * key = keys + i * table->key_size;
    void * start = table->key_area->ptr + *idx * table->key_size;
    void * end = table->key_area->ptr + table->key_area->size;
    if(start >= end)
      break;
    u64 size = end - start;
    void * key_index = NULL;
    int firstcmp = table->cmp(key, start);
    if(firstcmp < 0) 
      continue; // start is bigger than key
    if(firstcmp == 0) 
      key_index = start; // no need to search.
    else
      key_index = memmem(start, size, key, table->key_size);
    //key_index = bsearch(key, start, size / table->key_size, table->key_size, table->cmp);
    if(key_index == NULL)
      continue;
    start = key_index;
    *idx = (start - table->key_area->ptr) / table->key_size;
    
    do{
      if(out_keys != NULL){
	ASSERT(orig_out_keys == out_keys);
	memcpy(out_keys, key, table->key_size);
	out_keys += table->key_size;
      }
      
      *indexes = *idx;
      indexes += 1;
      cnt -= 1;
      start += table->key_size;
      *idx += 1; 
    }while(table->cmp(start, key) == 0 && cnt > 0);

  }
  return orig_cnt - cnt;
}

void sorttable_clear(sorttable * area){
  mem_area_realloc(area->key_area, area->key_size);		
  mem_area_realloc(area->value_area, area->value_size);     
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
  {
    sorttable tabl = create_sorttable(sizeof(u64), sizeof(vec4), "mymultitab");
    tabl.is_multi_table = true;
    u64 key1 = 61;
    vec4 value = vec4_new(14, 15, 16, 17);
    sorttable_inserts(&tabl, &key1, &value, 1);
    value = vec4_new(1, 1, 2, 3);
    sorttable_inserts(&tabl, &key1, &value, 1);

    u64 keys[] = {61, 61, 61, 61, 61, 61, 61, 67};
    u64 indexes[array_count(keys)];
    sorttable_finds(&tabl,keys, indexes, array_count(keys));
    for(u64 i = 0; i < array_count(keys); i++)
      logd(" %i ", indexes[i]);
    logd("\n");
    //sorttable_inserts(&tabl, &key1, &value, 1);
    //sorttable_inserts(&tabl, &key1, &value, 1);

    logd("All keys:\n");
    u64 * keypt = tabl.key_area->ptr;
    vec4 * vecs = tabl.value_area->ptr;
    for(u64 i = 0; i < tabl.key_area->size / tabl.key_size; i++){
      logd("%i %i :", i, keypt[i]);vec4_print(vecs[i]); logd(" == "); vec4_print(vec4_new(i, i * 2,keypt[i], keypt[i])); logd("\n");
    }
    u64 to_remove[] = {61, 61, 61, 61};
    sorttable_removes(&tabl, to_remove, array_count(to_remove));
    ASSERT(tabl.key_area->size == tabl.key_size);
    
  }
  {
    u64 arraything[] = {1,4,6,7,7,7,7,7,7,7,7,7,7,8,8,9,19,20,111};
    u64 key = 111;
    u64 * x = bsearch_bigger((void *) keycmp,&key , arraything, arraything + sizeof(arraything), sizeof(arraything[0]));
    logd("%i \n", *x);
    ASSERT(x == arraything + array_count(arraything));
  }
}
