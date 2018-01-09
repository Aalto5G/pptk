#ifndef _TIMERLINK_H_
#define _TIMERLINK_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

struct timer_linkheap;
struct timer_link;

typedef void (*timer_link_fn)(struct timer_link *timer, struct timer_linkheap *heap, void *userdata);

struct timer_link {
  uint64_t time64;
  timer_link_fn fn;
  void *userdata;
  struct timer_link *parent;
  struct timer_link *left;
  struct timer_link *right;
};

struct timer_linkheap {
  struct timer_link *root;
  size_t size;
};

void timer_linkheap_siftdown(
  struct timer_linkheap *heap, struct timer_link *link);

void timer_linkheap_siftup(
  struct timer_linkheap *heap, struct timer_link *link);

int timer_linkheap_verify(struct timer_linkheap *heap);

int timer_linkheap_grow(struct timer_linkheap *heap);

#define TIMER_LINKHEAP_INITER { \
  .root = NULL, \
  .size = 0, \
}

static inline uint64_t timer_linkheap_next_expiry_time(struct timer_linkheap *heap)
{
  if (heap->root == NULL)
  {
    return UINT64_MAX;
  }
  return heap->root->time64;
}

static inline struct timer_link *timer_linkheap_next_expiry_timer(struct timer_linkheap *heap)
{
  if (heap->root == NULL)
  {
    return NULL;
  }
  return heap->root;
}

static inline void timer_linkheap_init(struct timer_linkheap *heap)
{
  heap->root = NULL;
  heap->size = 0;
}

static inline void timer_linkheap_free(struct timer_linkheap *heap)
{
  if (heap->root != NULL || heap->size != 0)
  {
    abort();
  }
#if 0
  heap->root = NULL;
  heap->size = 0;
#endif
}

void timer_linkheap_add(struct timer_linkheap *heap, struct timer_link *timer);

void timer_linkheap_remove(struct timer_linkheap *heap, struct timer_link *timer);

void timer_linkheap_modify(struct timer_linkheap *heap, struct timer_link *timer);

#endif
