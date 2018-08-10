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
#include "timerlink.h"

struct timer_linkwheel {
  struct timer_linkheap *timers;
  pthread_mutex_t *locks;
  uint32_t curidx;
  uint32_t size;
  uint32_t granularity;
  uint64_t next_time64;
};

struct timer_linkwheel_task {
  struct timer_link link;
  uint32_t idx;
};

struct timer_linkwheel_threaddata {
  struct timer_linkwheel *linkwheel;
  void *orig_threaddata;
};

static inline int timer_linkwheel_init(struct timer_linkwheel *wheel, uint32_t granularity, uint32_t size, uint64_t next_time64, int locked)
{
  uint32_t i;
  wheel->timers = malloc(sizeof(*wheel->timers) * size);
  if (wheel->timers == NULL)
  {
    return -ENOMEM;
  }
  for (i = 0; i < size; i++)
  {
    timer_linkheap_init(&wheel->timers[i]);
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

void timer_linkwheel_add(struct timer_linkwheel *wheel, struct timer_linkwheel_task *timer);

static inline uint64_t timer_linkwheel_next_expiry_time(struct timer_linkwheel *wheel)
{
  return wheel->next_time64;
}

static inline void timer_linkwheel_process(struct timer_linkwheel *wheel, uint64_t curtime64, void *threaddata)
{
  struct timer_linkwheel_threaddata td = {
    .linkwheel = wheel,
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
      struct timer_link *link =
        timer_linkheap_next_expiry_timer(&wheel->timers[wheel->curidx]);
      if (link == NULL ||
          link->time64 >= wheel->next_time64 + wheel->granularity)
      {
        break;
      }
      timer_linkheap_remove(&wheel->timers[wheel->curidx], link);
      link->fn(link, NULL, link->userdata, &td);
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

static inline void timer_linkwheel_remove(struct timer_linkwheel *wheel, struct timer_linkwheel_task *timer)
{
  uint32_t idx = timer->idx;
  if (wheel->locks)
  {
    if (pthread_mutex_lock(&wheel->locks[idx]) != 0)
    {
      abort();
    }
  }
  timer_linkheap_remove(&wheel->timers[idx], &timer->link);
  if (wheel->locks)
  {
    if (pthread_mutex_unlock(&wheel->locks[idx]) != 0)
    {
      abort();
    }
  }
}

static inline void timer_linkwheel_modify(struct timer_linkwheel *wheel, struct timer_linkwheel_task *timer)
{
  timer_linkwheel_remove(wheel, timer);
  timer_linkwheel_add(wheel, timer);
}

#endif
