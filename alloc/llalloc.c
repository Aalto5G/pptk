#include "llalloc.h"
#include "allocif.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

static void *ll_allocif_mt(struct allocif *intf, size_t sz)
{
  struct ll_alloc_local *loc = intf->userdata;
  return ll_alloc_mt(loc, sz);
}

static void ll_freeif_mt(struct allocif *intf, void *block)
{
  struct ll_alloc_local *loc = intf->userdata;
  ll_free_mt(loc, block);
}

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

const struct allocif_ops ll_allocif_ops_mt = {
  .alloc = ll_allocif_mt,
  .free = ll_freeif_mt,
};

const struct allocif_ops ll_allocif_ops_st = {
  .alloc = ll_allocif_st,
  .free = ll_freeif_st,
};

int ll_alloc_st_init(
  struct ll_alloc_st *st, size_t capacity, size_t native_size)
{
  if (native_size < sizeof(struct linked_list_node))
  {
    native_size = sizeof(struct linked_list_node);
  }
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

int ll_alloc_global_init(
  struct ll_alloc_global *glob, size_t capacity, size_t native_size)
{
  if (native_size < sizeof(struct linked_list_node))
  {
    native_size = sizeof(struct linked_list_node);
  }
  glob->capacity = 0;
  glob->size = 0;
  glob->native_size = 0;
  linked_list_head_init(&glob->cache);
  if (pthread_mutex_init(&glob->mtx, NULL) != 0)
  {
    return -ENOMEM;
  }
  glob->capacity = capacity;
  glob->native_size = native_size;
  return 0;
}

void ll_alloc_global_free(struct ll_alloc_global *glob)
{
  while (!linked_list_is_empty(&glob->cache))
  {
    struct linked_list_node *node = glob->cache.node.next;
    linked_list_delete(node);
    free(alloc_allocated_block(node));
    glob->size--;
  }
  pthread_mutex_destroy(&glob->mtx);
  glob->capacity = 0;
  glob->size = 0;
  glob->native_size = 0;
}

int ll_alloc_local_init(
  struct ll_alloc_local *loc, struct ll_alloc_global *glob, size_t capacity)
{
  loc->capacity = 0;
  loc->size = 0;
  loc->glob = NULL;
  loc->native_size = 0;
  linked_list_head_init(&loc->cache);
  loc->glob = glob;
  loc->capacity = capacity;
  loc->native_size = glob->native_size;
  return 0;
}

void ll_alloc_local_free(struct ll_alloc_local *loc)
{
  while (!linked_list_is_empty(&loc->cache))
  {
    struct linked_list_node *node = loc->cache.node.next;
    linked_list_delete(node);
    free(alloc_allocated_block(node));
    loc->size--;
  }
  loc->glob = NULL;
  loc->capacity = 0;
  loc->size = 0;
  loc->native_size = 0;
}

void ll_alloc_fill(struct ll_alloc_local *loc)
{
  struct ll_alloc_global *glob = loc->glob;
  if (pthread_mutex_lock(&glob->mtx) != 0)
  {
    abort();
  }
  while (loc->size < loc->capacity/2 && glob->size > 0)
  {
    struct linked_list_node *node = glob->cache.node.next;
    linked_list_delete(node);
    glob->size--;
    linked_list_add_tail(node, &loc->cache);
    loc->size++;
  }
  if (pthread_mutex_unlock(&glob->mtx) != 0)
  {
    abort();
  }
}

void ll_alloc_halve(struct ll_alloc_local *loc)
{
  struct ll_alloc_global *glob = loc->glob;
  if (pthread_mutex_lock(&glob->mtx) != 0)
  {
    abort();
  }
  while (loc->size > loc->capacity/2 && glob->size < glob->capacity)
  {
    struct linked_list_node *node = loc->cache.node.next;
    linked_list_delete(node);
    loc->size--;
    linked_list_add_tail(node, &glob->cache);
    glob->size++;
  }
  if (pthread_mutex_unlock(&glob->mtx) != 0)
  {
    abort();
  }
  while (loc->size > loc->capacity/2)
  {
    struct linked_list_node *node = loc->cache.node.next;
    linked_list_delete(node);
    loc->size--;
    free(alloc_allocated_block(node));
  }
}
