#include "asalloc.h"
#include "allocif.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

static void *as_allocif_mt(struct allocif *intf, size_t sz)
{
  struct as_alloc_local *loc = intf->userdata;
  return as_alloc_mt(loc, sz);
}

static void as_freeif_mt(struct allocif *intf, void *block)
{
  struct as_alloc_local *loc = intf->userdata;
  as_free_mt(loc, block);
}

static void *as_allocif_st(struct allocif *intf, size_t sz)
{
  struct as_alloc_st *st = intf->userdata;
  return as_alloc_st(st, sz);
}

static void as_freeif_st(struct allocif *intf, void *block)
{
  struct as_alloc_st *st = intf->userdata;
  as_free_st(st, block);
}

const struct allocif_ops as_allocif_ops_mt = {
  .alloc = as_allocif_mt,
  .free = as_freeif_mt,
};

const struct allocif_ops as_allocif_ops_st = {
  .alloc = as_allocif_st,
  .free = as_freeif_st,
};

int as_alloc_st_init(
  struct as_alloc_st *st, size_t capacity, size_t native_size)
{
  st->capacity = 0;
  st->size = 0;
  st->native_size = 0;
  st->cache = NULL;
  st->cache = malloc(sizeof(*st->cache)*capacity);
  if (st->cache == NULL)
  {
    return -ENOMEM;
  }
  st->capacity = capacity;
  st->native_size = native_size;
  return 0;
}

void as_alloc_st_free(struct as_alloc_st *st)
{
  while (st->size > 0)
  {
    free(st->cache[st->size - 1]);
    st->size--;
  }
  free(st->cache);
  st->cache = NULL;
  st->capacity = 0;
  st->size = 0;
  st->native_size = 0;
}

int as_alloc_global_init(
  struct as_alloc_global *glob, size_t capacity, size_t native_size)
{
  glob->capacity = 0;
  glob->size = 0;
  glob->native_size = 0;
  glob->cache = NULL;
  glob->cache = malloc(sizeof(*glob->cache)*capacity);
  if (glob->cache == NULL)
  {
    return -ENOMEM;
  }
  if (pthread_mutex_init(&glob->mtx, NULL) != 0)
  {
    free(glob->cache);
    glob->cache = NULL;
    return -ENOMEM;
  }
  glob->capacity = capacity;
  glob->native_size = native_size;
  return 0;
}

void as_alloc_global_free(struct as_alloc_global *glob)
{
  while (glob->size > 0)
  {
    free(glob->cache[glob->size - 1]);
    glob->size--;
  }
  free(glob->cache);
  pthread_mutex_destroy(&glob->mtx);
  glob->cache = NULL;
  glob->capacity = 0;
  glob->size = 0;
  glob->native_size = 0;
}

int as_alloc_local_init(
  struct as_alloc_local *loc, struct as_alloc_global *glob, size_t capacity)
{
  loc->capacity = 0;
  loc->size = 0;
  loc->cache = NULL;
  loc->glob = NULL;
  loc->native_size = 0;
  loc->cache = malloc(sizeof(*loc->cache)*capacity);
  if (loc->cache == NULL)
  {
    return -ENOMEM;
  }
  loc->glob = glob;
  loc->capacity = capacity;
  loc->native_size = glob->native_size;
  return 0;
}

void as_alloc_local_free(struct as_alloc_local *loc)
{
  while (loc->size > 0)
  {
    free(loc->cache[loc->size - 1]);
    loc->size--;
  }
  free(loc->cache);
  loc->cache = NULL;
  loc->glob = NULL;
  loc->capacity = 0;
  loc->size = 0;
  loc->native_size = 0;
}

void as_alloc_fill(struct as_alloc_local *loc)
{
  struct as_alloc_global *glob = loc->glob;
  if (pthread_mutex_lock(&glob->mtx) != 0)
  {
    abort();
  }
  while (loc->size < loc->capacity/2 && glob->size > 0)
  {
    loc->cache[loc->size++] = glob->cache[glob->size - 1];
    glob->cache[--glob->size] = NULL;;
  }
  if (pthread_mutex_unlock(&glob->mtx) != 0)
  {
    abort();
  }
}

void as_alloc_halve(struct as_alloc_local *loc)
{
  struct as_alloc_global *glob = loc->glob;
  if (pthread_mutex_lock(&glob->mtx) != 0)
  {
    abort();
  }
  while (loc->size > loc->capacity/2 && glob->size < glob->capacity)
  {
    glob->cache[glob->size++] = loc->cache[loc->size - 1];
    loc->cache[--loc->size] = NULL;
  }
  if (pthread_mutex_unlock(&glob->mtx) != 0)
  {
    abort();
  }
  while (loc->size > loc->capacity/2)
  {
    free(loc->cache[loc->size - 1]);
    loc->cache[loc->size - 1] = NULL;
    loc->size--;
  }
}
