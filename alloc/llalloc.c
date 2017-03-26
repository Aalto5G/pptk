#include "llalloc.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

int ll_alloc_st_init(
  struct ll_alloc_st *st, size_t capacity, size_t native_size)
{
  st->capacity = 0;
  st->size = 0;
  st->native_size = 0;
  linked_list_head_init(&st->cache);
  st->capacity = capacity;
  st->native_size = native_size;
  return 0;
}

void ll_alloc_st_free(struct ll_alloc_st *st)
{
  while (!linked_list_is_empty(&st->cache))
  {
    struct linked_list_node *node = st->cache.node.next;
    linked_list_delete(node);
    free(alloc_allocated_block(node));
    st->size--;
  }
  st->capacity = 0;
  st->size = 0;
  st->native_size = 0;
}
