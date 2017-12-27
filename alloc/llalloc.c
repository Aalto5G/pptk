#include "llalloc.h"
#include "allocif.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

static void *ll_allocif_st(struct allocif *intf, size_t sz)
{
  struct ll_alloc_st *st = intf->userdata;
  return ll_alloc_st(st, sz);
}

static void ll_freeif_st(struct allocif *intf, void *block)
{
  struct ll_alloc_st *st = intf->userdata;
  ll_free_st(st, block);
}

const struct allocif_ops ll_allocif_ops_st = {
  .alloc = ll_allocif_st,
  .free = ll_freeif_st,
};

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
