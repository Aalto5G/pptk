#ifndef _ASALLOC_H_
#define _ASALLOC_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "generalalloc.h"

struct as_alloc_global {
  pthread_mutex_t mtx;
  void **cache;
  size_t capacity;
  size_t size;
  size_t native_size;
};

struct as_alloc_local {
  struct as_alloc_global *glob;
  void **cache;
  size_t capacity;
  size_t size;
  size_t native_size;
};

struct as_alloc_st {
  void **cache;
  size_t capacity;
  size_t size;
  size_t native_size;
};

int as_alloc_st_init(
  struct as_alloc_st *st, size_t capacity, size_t native_size);

void as_alloc_st_free(struct as_alloc_st *st);

int as_alloc_global_init(
  struct as_alloc_global *glob, size_t capacity, size_t native_size);

void as_alloc_global_free(struct as_alloc_global *glob);

int as_alloc_local_init(
  struct as_alloc_local *loc, struct as_alloc_global *glob, size_t capacity);

void as_alloc_local_free(struct as_alloc_local *loc);

void as_alloc_fill(struct as_alloc_local *loc);

void as_alloc_halve(struct as_alloc_local *loc);

static inline void *as_alloc_mt(struct as_alloc_local *loc, size_t size)
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
  if (loc->size == 0)
  {
    as_alloc_fill(loc);
  }
  if (loc->size > 0)
  {
    void *result = ((char*)loc->cache[loc->size - 1]) + sizeof(size_t);
    loc->cache[--loc->size] = NULL;
    return result;
  }
  buf = malloc(size + sizeof(size_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf, &size, sizeof(size_t));
  return buf + sizeof(size_t);
}

static inline void as_free_mt(struct as_alloc_local *loc, void *buf)
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
  if (loc->size >= loc->capacity)
  {
    as_alloc_halve(loc);
  }
  loc->cache[loc->size++] = allocated_block;
}

static inline void *as_alloc_st(struct as_alloc_st *st, size_t size)
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
  if (st->size > 0)
  {
    void *result = ((char*)st->cache[st->size - 1]) + sizeof(size_t);
    st->cache[--st->size] = NULL;
    return result;
  }
  buf = malloc(size + sizeof(size_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf, &size, sizeof(size_t));
  return buf + sizeof(size_t);
}

static inline void as_free_st(struct as_alloc_st *st, void *buf)
{
  size_t size;
  void *allocated_block;
  if (buf == NULL)
  {
    return;
  }
  allocated_block = alloc_allocated_block(buf);
  memcpy(&size, allocated_block, sizeof(size_t));
  if (size != st->native_size || st->size >= st->capacity)
  {
    free(allocated_block);
    return;
  }
  st->cache[st->size++] = allocated_block;
}

extern const struct allocif_ops as_allocif_ops_mt;
extern const struct allocif_ops as_allocif_ops_st;


#endif
