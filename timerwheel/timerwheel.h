#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include "hashlist.h"
#include "containerof.h"

struct timer_wheel;
struct timer_wheel_task;

typedef void (*timer_wheel_fn)(struct timer_wheel_task *timer, struct timer_wheel *wheel, void *userdata);

struct timer_wheel_task {
  struct hash_list_node node;
  uint64_t time64;
  uint32_t rotation_count;
  timer_wheel_fn fn;
  void *userdata;
};

struct timer_wheel {
  struct hash_list_head *timers;
  uint32_t curidx;
  uint32_t size;
  uint32_t granularity;
  uint64_t next_time64;
};

static inline int timer_wheel_init(struct timer_wheel *wheel, uint32_t granularity, uint32_t size, uint64_t next_time64)
{
  uint32_t i;
  wheel->timers = malloc(sizeof(*wheel->timers) * size);
  if (wheel->timers == NULL)
  {
    return -ENOMEM;
  }
  for (i = 0; i < size; i++)
  {
    hash_list_head_init(&wheel->timers[i]);
  }
  wheel->curidx = 0;
  wheel->size = size;
  wheel->granularity = granularity;
  wheel->next_time64 = next_time64;
  return 0;
}

void timer_wheel_add(struct timer_wheel *wheel, struct timer_wheel_task *timer);

static inline uint64_t timer_wheel_next_expiry_time(struct timer_wheel *wheel)
{
  return wheel->next_time64;
}

static inline void timer_wheel_process(struct timer_wheel *wheel, uint64_t curtime64)
{
  while (curtime64 >= wheel->next_time64)
  {
    struct hash_list_node *node, *tmp;
    HASH_LIST_FOR_EACH_SAFE(node, tmp, &wheel->timers[wheel->curidx])
    {
      struct timer_wheel_task *task =
        CONTAINER_OF(node, struct timer_wheel_task, node);
      if (task->rotation_count > 0)
      {
        task->rotation_count--;
      }
      else
      {
        hash_list_delete(&task->node);
        task->fn(task, wheel, task->userdata);
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

static inline void timer_wheel_remove(struct timer_wheel *wheel, struct timer_wheel_task *timer)
{
  hash_list_delete(&timer->node);
}

static inline void timer_wheel_modify(struct timer_wheel *wheel, struct timer_wheel_task *timer)
{
  timer_wheel_remove(wheel, timer);
  timer_wheel_add(wheel, timer);
}

#endif
