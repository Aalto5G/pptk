#ifndef _TIMERLINKWHEEL_H_
#define _TIMERLINKWHEEL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include "hashlist.h"
#include "containerof.h"
#include "timerrb.h"

struct timer_rbwheel {
  struct timerrb *timers;
  pthread_mutex_t *locks;
  uint32_t curidx;
  uint32_t size;
  uint32_t granularity;
  uint64_t next_time64;
};

struct timer_rbwheel_task {
  struct rbtimer rb;
  uint32_t idx;
};

struct timer_rbwheel_threaddata {
  struct timer_rbwheel *rbwheel;
  void *orig_threaddata;
};

static inline int timer_rbwheel_init(struct timer_rbwheel *wheel, uint32_t granularity, uint32_t size, uint64_t next_time64, int locked)
{
  uint32_t i;
  wheel->timers = malloc(sizeof(*wheel->timers) * size);
  if (wheel->timers == NULL)
  {
    return -ENOMEM;
  }
  for (i = 0; i < size; i++)
  {
    timerrb_init(&wheel->timers[i]);
  }
  wheel->locks = NULL;
  if (locked)
  {
    wheel->locks = malloc(sizeof(*wheel->locks) * size);
    if (wheel->locks == NULL)
    {
      free(wheel->timers);
      wheel->timers = NULL;
      return -ENOMEM;
    }
    for (i = 0; i < size; i++)
    {
      if (pthread_mutex_init(&wheel->locks[i], NULL) != 0)
      {
        for (;;)
        {
          if (i == 0)
          {
            free(wheel->timers);
            free(wheel->locks);
            wheel->timers = NULL;
            wheel->locks = NULL;
            return -ENOMEM;
          }
          i--;
          pthread_mutex_destroy(&wheel->locks[i]);
        }
      }
    }
  }
  wheel->curidx = 0;
  wheel->size = size;
  wheel->granularity = granularity;
  wheel->next_time64 = next_time64;
  return 0;
}

void timer_rbwheel_add(struct timer_rbwheel *wheel, struct timer_rbwheel_task *timer);

static inline uint64_t timer_rbwheel_next_expiry_time(struct timer_rbwheel *wheel)
{
  return wheel->next_time64;
}

static inline void timer_rbwheel_process(struct timer_rbwheel *wheel, uint64_t curtime64, void *threaddata)
{
  struct timer_rbwheel_threaddata td = {
    .rbwheel = wheel,
    .orig_threaddata = threaddata,
  };
  while (curtime64 >= wheel->next_time64)
  {
    if (wheel->locks)
    {
      if (pthread_mutex_lock(&wheel->locks[wheel->curidx]) != 0)
      {
        abort();
      }
    }
    for (;;)
    {
      struct rbtimer *rb =
        timerrb_next_expiry_timer(&wheel->timers[wheel->curidx]);
      if (rb == NULL ||
          rb->time64 >= wheel->next_time64 + wheel->granularity)
      {
        break;
      }
      timerrb_remove(&wheel->timers[wheel->curidx], rb);
      rb->fn(rb, NULL, rb->userdata, &td);
    }
    if (wheel->locks)
    {
      if (pthread_mutex_unlock(&wheel->locks[wheel->curidx]) != 0)
      {
        abort();
      }
    }
    wheel->next_time64 += wheel->granularity;
    wheel->curidx++;
    if (wheel->curidx >= wheel->size)
    {
      wheel->curidx = 0;
    }
  }
}

static inline void timer_rbwheel_remove(struct timer_rbwheel *wheel, struct timer_rbwheel_task *timer)
{
  uint32_t idx = timer->idx;
  if (wheel->locks)
  {
    if (pthread_mutex_lock(&wheel->locks[idx]) != 0)
    {
      abort();
    }
  }
  timerrb_remove(&wheel->timers[idx], &timer->rb);
  if (wheel->locks)
  {
    if (pthread_mutex_unlock(&wheel->locks[idx]) != 0)
    {
      abort();
    }
  }
}

static inline void timer_rbwheel_modify(struct timer_rbwheel *wheel, struct timer_rbwheel_task *timer)
{
  timer_rbwheel_remove(wheel, timer);
  timer_rbwheel_add(wheel, timer);
}

#endif
