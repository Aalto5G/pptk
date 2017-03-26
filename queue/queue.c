#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include "queue.h"

int queue_cache_init(
  struct queue_cache *cache, struct queue *queue, size_t capacity)
{
  cache->queue = NULL;
  cache->buf = NULL;
  cache->capacity = 0;
  cache->size = 0;
  cache->buf = malloc(sizeof(*cache->buf)*capacity);
  if (cache->buf == NULL)
  {
    return -ENOMEM;
  }
  cache->capacity = capacity;
  cache->queue = queue;
  return 0;
}

int queue_init(struct queue *queue, size_t bufsiz)
{
  queue->buf = NULL;
  queue->bufsiz = 0;
  queue->start = 0;
  queue->end = 0;
  queue->buf = malloc(sizeof(*queue->buf)*bufsiz);
  if (pthread_mutex_init(&queue->mtx, NULL) != 0)
  {
    goto mtx_fail;
  }
  if (pthread_cond_init(&queue->full, NULL) != 0)
  {
    goto cond1_fail;
  }
  if (pthread_cond_init(&queue->empty, NULL) != 0)
  {
    goto cond2_fail;
  }
  queue->bufsiz = bufsiz;
  return 0;

cond2_fail:
  pthread_cond_destroy(&queue->full);
cond1_fail:
  pthread_mutex_destroy(&queue->mtx);
mtx_fail:
  free(queue->buf);
  queue->buf = NULL;
  return -ENOMEM;
}

void queue_enq_one(struct queue *queue, void *item)
{
  int was_empty;
  if (pthread_mutex_lock(&queue->mtx) != 0)
  {
    abort();
  }
  while (queue_is_full(queue))
  {
    if (pthread_cond_wait(&queue->full, &queue->mtx) != 0)
    {
      abort();
    }
  }
  was_empty = queue_is_empty(queue);
  queue->buf[queue->end++] = item;
  if (queue->end >= queue->bufsiz)
  {
    queue->end = 0;
  }
  if (was_empty)
  {
    if (pthread_cond_broadcast(&queue->empty) != 0)
    {
      abort();
    }
  }
  if (pthread_mutex_unlock(&queue->mtx) != 0)
  {
    abort();
  }
}

void *queue_deq_one(struct queue *queue)
{
  int was_full;
  void *item;
  if (pthread_mutex_lock(&queue->mtx) != 0)
  {
    abort();
  }
  while (queue_is_empty(queue))
  {
    if (pthread_cond_wait(&queue->empty, &queue->mtx) != 0)
    {
      abort();
    }
  }
  was_full = queue_is_full(queue);
  item = queue->buf[queue->start++];
  if (queue->start >= queue->bufsiz)
  {
    queue->start = 0;
  }
  if (was_full)
  {
    if (pthread_cond_broadcast(&queue->full) != 0)
    {
      abort();
    }
  }
  if (pthread_mutex_unlock(&queue->mtx) != 0)
  {
    abort();
  }
  return item;
}

void queue_enq_many(struct queue *queue, void **in, size_t sz)
{
  int was_empty;
  size_t i = 0;
  if (pthread_mutex_lock(&queue->mtx) != 0)
  {
    abort();
  }
  while (i < sz)
  {
    while (queue_is_full(queue))
    {
      if (pthread_cond_wait(&queue->full, &queue->mtx) != 0)
      {
        abort();
      }
    }
    was_empty = queue_is_empty(queue);
    queue->buf[queue->end++] = in[i++];
    if (queue->end >= queue->bufsiz)
    {
      queue->end = 0;
    }
    if (was_empty)
    {
      if (pthread_cond_broadcast(&queue->empty) != 0)
      {
        abort();
      }
    }
  }
  if (pthread_mutex_unlock(&queue->mtx) != 0)
  {
    abort();
  }
}

size_t queue_deq_many(struct queue *queue, void **out, size_t sz)
{
  int was_full;
  size_t i = 0;
  if (pthread_mutex_lock(&queue->mtx) != 0)
  {
    abort();
  }
  while (queue_is_empty(queue))
  {
    if (pthread_cond_wait(&queue->empty, &queue->mtx) != 0)
    {
      abort();
    }
  }
  was_full = queue_is_full(queue);
  while (!queue_is_empty(queue) && i < sz)
  {
    out[i++] = queue->buf[queue->start++];
    if (queue->start >= queue->bufsiz)
    {
      queue->start = 0;
    }
  }
  if (was_full)
  {
    if (pthread_cond_broadcast(&queue->full) != 0)
    {
      abort();
    }
  }
  if (pthread_mutex_unlock(&queue->mtx) != 0)
  {
    abort();
  }
  return i;
}

size_t queue_timeddeq_many(
  struct queue *queue, void **out, size_t sz, struct timespec *ts)
{
  int was_full;
  size_t i = 0;
  if (pthread_mutex_lock(&queue->mtx) != 0)
  {
    abort();
  }
  while (queue_is_empty(queue))
  {
    int status;
    status = pthread_cond_timedwait(&queue->empty, &queue->mtx, ts);
    if (status == ETIMEDOUT)
    {
      goto out;
    }
    else if (status != 0)
    {
      abort();
    }
  }
  was_full = queue_is_full(queue);
  while (!queue_is_empty(queue) && i < sz)
  {
    out[i++] = queue->buf[queue->start++];
    if (queue->start >= queue->bufsiz)
    {
      queue->start = 0;
    }
  }
  if (was_full)
  {
    if (pthread_cond_broadcast(&queue->full) != 0)
    {
      abort();
    }
  }
out:
  if (pthread_mutex_unlock(&queue->mtx) != 0)
  {
    abort();
  }
  return i;
}
