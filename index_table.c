#include <iron/full.h>
#include "persist.h"
#include "index_table.h"
u32 index_table_capacity(index_table * table){
  return table->area->size / table->element_size - 4;
}

u32 index_table_count(index_table * table){
  return ((u32 *) table->area->ptr)[0];
}

void index_table_count_set(index_table * table, u32 newcount){
  ((u32 *) table->area->ptr)[0] = newcount;
}

u32 _index_table_free_index_count(index_table * table){
  return ((u32 *) table->free_indexes->ptr)[0];
}

void _index_table_free_index_count_set(index_table * table, u32 cnt){
  ((u32 *) table->free_indexes->ptr)[0] = cnt;
}
void * index_table_all(index_table * table, u64 * cnt){
  // return count -1 and a pointer from the placeholder element.
  *cnt = index_table_count(table) - 1;
  return table->area->ptr + table->element_size * 5;
}
void index_table_clear(index_table * table){
  index_table_count_set(table, 1);
  _index_table_free_index_count_set(table, 0);
}

u32 _index_table_alloc(index_table * table){
  u32 freeindexcnt = _index_table_free_index_count(table);
  if(freeindexcnt > 0){
    u32 idx = ((u32 *) table->free_indexes->ptr)[freeindexcnt];
    _index_table_free_index_count_set(table, freeindexcnt - 1);
    ASSERT(idx != 0);
    void * p = index_table_lookup(table, idx);
    memset(p, 0, table->element_size);
    return idx;
  }
  
  while(index_table_capacity(table) <= index_table_count(table)){
    u32 prevsize = table->area->size;
    ASSERT((prevsize % table->element_size) == 0);
    u32 newsize = MAX(prevsize * 2, 8 * table->element_size);
    mem_area_realloc(table->area, newsize);
    memset(table->area->ptr + prevsize, 0,  newsize - prevsize);
  }
  u32 idx = index_table_count(table);
  index_table_count_set(table,idx + 1);
  return idx;
}

u32 index_table_alloc(index_table * table){
  auto index = _index_table_alloc(table);
  ASSERT(index > 0);
  return index;
}


void index_table_remove(index_table * table, u32 index){
  ASSERT(index < index_table_count(table));
  ASSERT(index > 0);
  u32 cnt = _index_table_free_index_count(table);
  mem_area_realloc(table->free_indexes, table->free_indexes->size + sizeof(u32));
  ((u32 *)table->free_indexes->ptr)[cnt + 1] = 0;
  //ASSERT(memmem(table->free_indexes->ptr + sizeof(u32), (cnt + 1) * sizeof(u32), &index, sizeof(index)) == NULL);
  ((u32 *)table->free_indexes->ptr)[cnt + 1] = index;
  ((u32 *) table->free_indexes->ptr)[0] += 1;
}

void index_table_resize_sequence(index_table * table, index_table_sequence * seq,  u32 new_count){
  index_table_sequence nseq = {0};
  if(new_count == 0){
    index_table_remove_sequence(table, seq);
    *seq = nseq;
    return;
  }
  
  nseq = index_table_alloc_sequence(table, new_count);
  
  if(seq->index != 0 && seq->count != 0 ){
    if(new_count > 0){
      void * src = index_table_lookup_sequence(table, *seq);
      void * dst = index_table_lookup_sequence(table, nseq);
      memmove(dst, src, MIN(seq->count, nseq.count) * table->element_size);
    }
    index_table_remove_sequence(table, seq);
  }
  *seq = nseq;
}

index_table_sequence index_table_alloc_sequence(index_table * table, u32 count){
  u32 freeindexcnt = _index_table_free_index_count(table);
  
  if(freeindexcnt > 0){
      u32 start = 0;
      u32 cnt = 0;
      for(u32 i = 0; i < freeindexcnt; i++){
	u32 idx = ((u32 *) table->free_indexes->ptr)[i + 1];
	if(start == 0){
	  start = idx;
	  cnt = 1;
	}else if(idx == start + cnt){
	  cnt += 1;
	}else if(idx == start + cnt - 1){
	  ASSERT(false); // it seems that some bug can appear here.
	}else{
	  start = idx;
	  cnt = 1;
	}
	if(cnt == count){
	  // pop it from the indexex.
	  _index_table_free_index_count_set(table, freeindexcnt - cnt);
	  u32 * ptr = table->free_indexes->ptr;
	  for(u32 j = i - cnt + 1; j < freeindexcnt - cnt; j++)
	    ptr[j + 1] = ptr[j + cnt + 1];
	  
	  ASSERT(start != 0);
	  void * p = index_table_lookup(table, start);
	  memset(p, 0, cnt * table->element_size);
	  return (index_table_sequence){.index = start, .count = cnt};
	}
      }
    }
  
  while(index_table_capacity(table) <= (index_table_count(table) + count)){
    u32 prevsize = table->area->size;
    ASSERT((prevsize % table->element_size) == 0);
    u32 newsize = MAX(prevsize * 2, 8 * table->element_size);
    mem_area_realloc(table->area, newsize);
    memset(table->area->ptr + prevsize, 0,  newsize - prevsize);
  }
  u32 idx = index_table_count(table);
  index_table_count_set(table,idx + count);
  return (index_table_sequence){.index = idx, .count = count};
}

void index_table_remove_sequence(index_table * table, index_table_sequence * seq){
  u32 cnt = _index_table_free_index_count(table);
  mem_area_realloc(table->free_indexes, table->free_indexes->size + seq->count * sizeof(u32));
  //ASSERT(memmem(table->free_indexes->ptr + sizeof(u32), cnt * sizeof(u32), &index, sizeof(index)) == NULL);
  for(u32 i = 0; i < seq->count; i++)
    ((u32 *)table->free_indexes->ptr)[cnt + i + 1] = seq->index + i;
  ((u32 *) table->free_indexes->ptr)[0] += seq->count;
  memset(seq, 0, sizeof(*seq));
  index_table_optimize(table);
}

void * index_table_lookup_sequence(index_table * table, index_table_sequence seq){
  if(seq.index == 0)
    return NULL;
  return index_table_lookup(table, seq.index);
}


index_table * index_table_create(const char * name, u32 element_size){
  ASSERT(element_size > 0);
  index_table table = {0};
  table.element_size = element_size;
  if(name != NULL){

    table.area = mem_area_create(name);
    char name2[128] = {0};
    sprintf(name2, "%s.free", name);
    table.free_indexes = mem_area_create(name2);
  }else{
    table.area = mem_area_create_non_persisted();
    table.free_indexes = mem_area_create_non_persisted();
  }
  
  if(table.free_indexes->size < sizeof(u32)){
    mem_area_realloc(table.free_indexes, sizeof(u32));
    ((u32 *)table.free_indexes->ptr)[0] = 0;
  }

  if(table.area->size < element_size){
    mem_area_realloc(table.area, element_size * 4);
    memset(table.area->ptr, 0, table.area->size);
    _index_table_alloc(&table);
  }
  ASSERT((table.area->size % table.element_size) == 0);
  ASSERT(index_table_count(&table) > 0);
  return iron_clone(&table, sizeof(table));
}


void index_table_destroy(index_table ** _table){
  index_table * table = *_table;
  *_table = NULL;
  mem_area_free(table->area);
  mem_area_free(table->free_indexes);
}

void * index_table_lookup(index_table * table, u32 index){
  ASSERT(index < index_table_count(table));
  ASSERT(index > 0);
  return table->area->ptr + (4 + index) * table->element_size;
}

bool index_table_contains(index_table * table, u32 index){
  if(index == 0)
    return false;
  if (index >= index_table_count(table))
    return false;
  u32 freecnt = _index_table_free_index_count(table);
  u32 * start = table->free_indexes->ptr + sizeof(u32);
  for(u32 i = 0; i < freecnt; i++){
    if(start[i] == index)
      return false;
  }
  return true;
}

void index_table_optimize(index_table * table){
  // the index table is optimized by sorting and removing excess elements
  //
  int cmpfunc (const u32 * a, const u32 * b)
  {
    return ( *(int*)a - *(int*)b );
  }
  
  u32 free_cnt = _index_table_free_index_count(table); 
  if(table->free_indexes->ptr == NULL || free_cnt ==  0)
    return; // nothing to optimize.
    
  u32 * p = table->free_indexes->ptr;
  qsort(p + 1,free_cnt , sizeof(u32), (void *) cmpfunc);
  u32 table_cnt = index_table_count(table);

  // remove all elements that are bigger than the table. note: how did these enter?
  while(p[free_cnt] >= table_cnt && free_cnt != 0){
    free_cnt -= 1;
  }

  // remove all elements that are freed from the end of the table
  while(table_cnt > 0 && p[free_cnt] == table_cnt - 1 && free_cnt != 0){
    table_cnt -= 1;
    free_cnt -= 1;
  }
  _index_table_free_index_count_set(table, free_cnt);
  index_table_count_set(table, table_cnt);
  u64 newsize = table_cnt * table->element_size + table->element_size * 5;
  mem_area_realloc(table->area, newsize);
  u64 newsize2 = free_cnt * table->element_size + sizeof(u32);
  mem_area_realloc(table->free_indexes, newsize2);
}
