#include "timerlink.h"
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "time64.h"

#define MIN_PERIOD (10*1000)
#define PERIOD_MUL 5
#define PERIOD_DIV 4

struct periodud {
  uint64_t period;
};

static void periodic_fn(
  struct timer_link *timer, struct timer_linkheap *heap, void *userdata,
  void *threaddata)
{
  struct periodud *ud = userdata;
#if 0
  if (!timer_linkheap_verify(heap))
  {
    abort();
  }
  printf("periodic timer, period %g\n", ud->period/1000.0/1000.0);
#endif
  timer->time64 = gettime64() + ud->period;
  timer_linkheap_add(heap, timer);
#if 0
  if (!timer_linkheap_verify(heap))
  {
    abort();
  }
#endif
}

int main(int argc, char **argv)
{
  struct timer_linkheap heap;
  struct periodud periodics_ud[50];
  struct timer_link periodics[50];
  size_t i;
  timer_linkheap_init(&heap);
  periodics_ud[0].period = MIN_PERIOD;
  periodics[0].time64 = gettime64() + MIN_PERIOD;
  periodics[0].fn = periodic_fn;
  periodics[0].userdata = &periodics_ud[0];
  timer_linkheap_add(&heap, &periodics[0]);
  for (i = 1; i < 50; i++)
  {
    periodics_ud[i].period = periodics_ud[i-1].period*PERIOD_MUL/PERIOD_DIV;
    periodics[i].time64 = gettime64() + periodics_ud[i].period;
    periodics[i].fn = periodic_fn;
    periodics[i].userdata = &periodics_ud[i];
    timer_linkheap_add(&heap, &periodics[i]);
  }

  alarm(60);

  for (;;)
  {
    struct timer_link *timer;
    while (gettime64() < timer_linkheap_next_expiry_time(&heap))
    {
      usleep(timer_linkheap_next_expiry_time(&heap) - gettime64());
      //usleep(100);
      // spin
    }
    timer = timer_linkheap_next_expiry_timer(&heap);
    timer_linkheap_remove(&heap, timer);
    timer->fn(timer, &heap, timer->userdata, NULL);

    i = rand()%50;
    periodics[i].time64 += periodics_ud[i].period;
    timer_linkheap_modify(&heap, &periodics[i]);

    i = rand()%50;
    timer_linkheap_remove(&heap, &periodics[i]);
    timer_linkheap_add(&heap, &periodics[i]);
  }
  return 0;
}
