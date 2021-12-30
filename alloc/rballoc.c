#include "rballoc.h"
#include "allocif.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

static void *rb_allocif_mt(struct allocif *intf, size_t sz)
{
  struct rb_alloc_local *loc = intf->userdata;
  return rb_alloc_mt(loc, sz);
}

static void rb_freeif_mt(struct allocif *intf, void *block)
{
  struct rb_alloc_local *loc = intf->userdata;
  rb_free_mt(loc, block);
}

static void *rb_allocif_st(struct allocif *intf, size_t sz)
{
  struct rb_alloc_st *st = intf->userdata;
  return rb_alloc_st(st, sz);
}

static void rb_freeif_st(struct allocif *intf, void *block)
{
  struct rb_alloc_st *st = intf->userdata;
  rb_free_st(st, block);
}

const struct allocif_ops rb_allocif_ops_mt = {
  .alloc = rb_allocif_mt,
  .free = rb_freeif_mt,
};

const struct allocif_ops rb_allocif_ops_st = {
  .alloc = rb_allocif_st,
  .free = rb_freeif_st,
};


int rb_alloc_st_init(
  struct rb_alloc_st *st, size_t capacity, size_t native_size)
{
  st->capacity = 0;
  st->start = 0;
  st->end = 0;
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

void rb_alloc_st_free(struct rb_alloc_st *st)
{
  while (!rb_alloc_st_empty(st))
  {
    free(st->cache[st->start++]);
    if (st->start >= st->capacity)
    {
      st->start = 0;
    }
  }
  free(st->cache);
  st->cache = NULL;
  st->capacity = 0;
  st->start = 0;
  st->end = 0;
  st->native_size = 0;
}

int rb_alloc_global_init(
  struct rb_alloc_global *glob, size_t capacity, size_t native_size)
{
  glob->capacity = 0;
  glob->start = 0;
  glob->end = 0;
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

void rb_alloc_global_free(struct rb_alloc_global *glob)
{
  while (!rb_alloc_global_empty(glob))
  {
    free(glob->cache[glob->start++]);
    if (glob->start >= glob->capacity)
    {
      glob->start = 0;
    }
  }
  free(glob->cache);
  pthread_mutex_destroy(&glob->mtx);
  glob->cache = NULL;
  glob->capacity = 0;
  glob->start = 0;
  glob->end = 0;
  glob->native_size = 0;
}

int rb_alloc_local_init(
  struct rb_alloc_local *loc, struct rb_alloc_global *glob, size_t capacity)
{
  loc->capacity = 0;
  loc->start = 0;
  loc->end = 0;
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

void rb_alloc_local_free(struct rb_alloc_local *loc)
{
  while (!rb_alloc_local_empty(loc))
  {
    free(loc->cache[loc->start++]);
    if (loc->start >= loc->capacity)
    {
      loc->start = 0;
    }
  }
  free(loc->cache);
  loc->cache = NULL;
  loc->glob = NULL;
  loc->capacity = 0;
  loc->start = 0;
  loc->end = 0;
  loc->native_size = 0;
}

static inline size_t rb_alloc_local_size(struct rb_alloc_local *loc)
{
  if (loc->end >= loc->start)
  {
    return loc->end - loc->start;
  }
  else
  {
    return loc->capacity + loc->end - loc->start;
  }
}

void rb_alloc_fill(struct rb_alloc_local *loc)
{
  struct rb_alloc_global *glob = loc->glob;
  if (pthread_mutex_lock(&glob->mtx) != 0)
  {
    abort();
  }
  while (    rb_alloc_local_size(loc) < loc->capacity/2
         && !rb_alloc_global_empty(glob))
  {
    loc->cache[loc->end++] = glob->cache[glob->start];
    glob->cache[glob->start++] = NULL;
    if (loc->end >= loc->capacity)
    {
      loc->end = 0;
    }
    if (glob->start >= glob->capacity)
    {
      glob->start = 0;
    }
  }
  if (pthread_mutex_unlock(&glob->mtx) != 0)
  {
    abort();
  }
}

void rb_alloc_halve(struct rb_alloc_local *loc)
{
  struct rb_alloc_global *glob = loc->glob;
  if (pthread_mutex_lock(&glob->mtx) != 0)
  {
    abort();
  }
  while (    rb_alloc_local_size(loc) > loc->capacity/2
         && !rb_alloc_global_full(glob))
  {
    glob->cache[glob->end++] = loc->cache[loc->start];
    loc->cache[loc->start++] = NULL;
    if (loc->start >= loc->capacity)
    {
      loc->start = 0;
    }
    if (glob->end >= glob->capacity)
    {
      glob->end = 0;
    }
  }
  if (pthread_mutex_unlock(&glob->mtx) != 0)
  {
    abort();
  }
  while (rb_alloc_local_size(loc) > loc->capacity/2)
  {
    free(loc->cache[loc->start]);
    loc->cache[loc->start++] = NULL;
    if (loc->start >= loc->capacity)
    {
      loc->start = 0;
    }
  }
}
