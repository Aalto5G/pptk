#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include "timerlinkwheel.h"

void timer_linkwheel_add(struct timer_linkwheel *wheel, struct timer_linkwheel_task *timer)
{
  int64_t diff = (((int64_t)timer->link.time64) - ((int64_t)wheel->next_time64))
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
  timer_linkheap_add(&wheel->timers[idx], &timer->link);
  if (wheel->locks)
  {
    if (pthread_mutex_unlock(&wheel->locks[idx]) != 0)
    {
      abort();
    }
  }
}

