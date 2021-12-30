#ifndef _RBALLOC_H_
#define _RBALLOC_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "generalalloc.h"

struct rb_alloc_global {
  pthread_mutex_t mtx;
  void **cache;
  size_t capacity;
  size_t start;
  size_t end;
  size_t native_size;
};

struct rb_alloc_local {
  struct rb_alloc_global *glob;
  void **cache;
  size_t capacity;
  size_t start;
  size_t end;
  size_t native_size;
};

struct rb_alloc_st {
  void **cache;
  size_t capacity;
  size_t start;
  size_t end;
  size_t native_size;
};

static inline int rb_alloc_st_empty(struct rb_alloc_st *st)
{
  return st->start == st->end;
}

static inline int rb_alloc_local_empty(struct rb_alloc_local *loc)
{
  return loc->start == loc->end;
}

static inline int rb_alloc_global_empty(struct rb_alloc_global *glob)
{
  return glob->start == glob->end;
}

static inline int rb_alloc_st_full(struct rb_alloc_st *st)
{
  size_t end_plus_1 = st->end + 1;
  if (end_plus_1 >= st->capacity)
  {
    end_plus_1 = 0;
  }
  return st->start == end_plus_1;
}

static inline int rb_alloc_local_full(struct rb_alloc_local *loc)
{
  size_t end_plus_1 = loc->end + 1;
  if (end_plus_1 >= loc->capacity)
  {
    end_plus_1 = 0;
  }
  return loc->start == end_plus_1;
}

static inline int rb_alloc_global_full(struct rb_alloc_global *glob)
{
  size_t end_plus_1 = glob->end + 1;
  if (end_plus_1 >= glob->capacity)
  {
    end_plus_1 = 0;
  }
  return glob->start == end_plus_1;
}

int rb_alloc_st_init(
  struct rb_alloc_st *st, size_t capacity, size_t native_size);

void rb_alloc_st_free(struct rb_alloc_st *st);

int rb_alloc_global_init(
  struct rb_alloc_global *glob, size_t capacity, size_t native_size);

void rb_alloc_global_free(struct rb_alloc_global *glob);

int rb_alloc_local_init(
  struct rb_alloc_local *loc, struct rb_alloc_global *glob, size_t capacity);

void rb_alloc_local_free(struct rb_alloc_local *loc);

void rb_alloc_fill(struct rb_alloc_local *loc);

void rb_alloc_halve(struct rb_alloc_local *loc);

static inline void *rb_alloc_mt(struct rb_alloc_local *loc, size_t size)
{
  char *buf;
  if (size > loc->native_size)
  {
    buf = malloc(size + sizeof(size_t));
    if (buf == NULL)
    {
      return NULL;
    }
    memcpy(buf, &size, sizeof(size_t));
    return buf + sizeof(size_t);
  }
  size = loc->native_size;
  if (rb_alloc_local_empty(loc))
  {
    rb_alloc_fill(loc);
  }
  if (!rb_alloc_local_empty(loc))
  {
    void *result = loc->cache[loc->start];
    loc->cache[loc->start++] = NULL;
    if (loc->start >= loc->capacity)
    {
      loc->start = 0;
    }
    return ((char*)result) + sizeof(size_t);
  }
  buf = malloc(size + sizeof(size_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf, &size, sizeof(size_t));
  return buf + sizeof(size_t);
}

static inline void rb_free_mt(struct rb_alloc_local *loc, void *buf)
{
  size_t size;
  void *allocated_block;
  if (buf == NULL)
  {
    return;
  }
  allocated_block = alloc_allocated_block(buf);
  memcpy(&size, allocated_block, sizeof(size_t));
  if (size != loc->native_size)
  {
    free(allocated_block);
    return;
  }
  if (rb_alloc_local_full(loc))
  {
    rb_alloc_halve(loc);
  }
  loc->cache[loc->end++] = allocated_block;
  if (loc->end >= loc->capacity)
  {
    loc->end = 0;
  }
}

static inline void *rb_alloc_st(struct rb_alloc_st *st, size_t size)
{
  char *buf;
  if (size > st->native_size)
  {
    buf = malloc(size + sizeof(size_t));
    if (buf == NULL)
    {
      return NULL;
    }
    memcpy(buf, &size, sizeof(size_t));
    return buf + sizeof(size_t);
  }
  size = st->native_size;
  if (!rb_alloc_st_empty(st))
  {
    void *result = st->cache[st->start];
    st->cache[st->start++] = NULL;
    if (st->start >= st->capacity)
    {
      st->start = 0;
    }
    return ((char*)result) + sizeof(size_t);
  }
  buf = malloc(size + sizeof(size_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf, &size, sizeof(size_t));
  return buf + sizeof(size_t);
}

static inline void rb_free_st(struct rb_alloc_st *st, void *buf)
{
  size_t size;
  void *allocated_block;
  if (buf == NULL)
  {
    return;
  }
  allocated_block = alloc_allocated_block(buf);
  memcpy(&size, allocated_block, sizeof(size_t));
  if (size != st->native_size || rb_alloc_st_full(st))
  {
    free(allocated_block);
    return;
  }
  st->cache[st->end++] = allocated_block;
  if (st->end >= st->capacity)
  {
    st->end = 0;
  }
}

extern const struct allocif_ops rb_allocif_ops_mt;
extern const struct allocif_ops rb_allocif_ops_st;

#endif
