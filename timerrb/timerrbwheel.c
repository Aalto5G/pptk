#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include "timerrbwheel.h"

void timer_rbwheel_add(struct timer_rbwheel *wheel, struct timer_rbwheel_task *timer)
{
  int64_t diff = (((int64_t)timer->rb.time64) - ((int64_t)wheel->next_time64))
                 / (int64_t)wheel->granularity;
  uint32_t idx;
  if (diff < 0)
  {
    diff = 0;
  }
  idx = (wheel->curidx + diff) % wheel->size;
  timer->idx = idx;
  if (wheel->locks)
  {
    if (pthread_mutex_lock(&wheel->locks[idx]) != 0)
    {
      abort();
    }
  }
  timerrb_add(&wheel->timers[idx], &timer->rb);
  if (wheel->locks)
  {
    if (pthread_mutex_unlock(&wheel->locks[idx]) != 0)
    {
      abort();
    }
  }
}

