#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

struct queue {
  void **buf;
  size_t bufsiz;
  size_t start;
  size_t end;
  pthread_mutex_t mtx;
  pthread_cond_t full;
  pthread_cond_t empty;
};

struct queue_cache {
  struct queue *queue;
  void **buf;
  size_t capacity;
  size_t size;
};

int queue_cache_init(
  struct queue_cache *cache, struct queue *queue, size_t capacity);

int queue_init(struct queue *queue, size_t bufsiz);

static inline int queue_is_empty(struct queue *queue)
{
  return queue->start == queue->end;
}

static inline int queue_is_full(struct queue *queue)
{
  size_t end_plus_1 = queue->end + 1;
  if (end_plus_1 >= queue->bufsiz)
  {
    end_plus_1 = 0;
  }
  return queue->start == end_plus_1;
}

void queue_enq_one(struct queue *queue, void *item);

void *queue_deq_one(struct queue *queue);

void queue_enq_many(struct queue *queue, void **in, size_t sz);

static inline void queue_cache_flush(struct queue_cache *cache)
{
  queue_enq_many(cache->queue, cache->buf, cache->size);
  cache->size = 0;
}

static inline void queue_cache_enq_one(struct queue_cache *cache, void *item)
{
  if (cache->size >= cache->capacity)
  {
    queue_cache_flush(cache);
  }
  cache->buf[cache->size++] = item;
}

size_t queue_deq_many(struct queue *queue, void **out, size_t sz);

#endif
