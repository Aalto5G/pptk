#include "timerlink.h"
#include <stdio.h>
#include <sys/time.h>
#include "time64.h"

#define MIN_PERIOD (1000*1000)
#define PERIOD_MUL 5
#define PERIOD_DIV 4

struct periodud {
  uint64_t period;
};

static void periodic_fn(
  struct timer_link *timer, struct timer_linkheap *heap, void *userdata)
{
  struct periodud *ud = userdata;
  if (!timer_linkheap_verify(heap))
  {
    printf("1 fail\n");
    abort();
  }
  printf("periodic timer, period %g\n", ud->period/1000.0/1000.0);
  timer->time64 = gettime64() + ud->period;
  timer_linkheap_add(heap, timer);
  if (!timer_linkheap_verify(heap))
  {
    printf("2 fail\n");
    abort();
  }
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
  if (!timer_linkheap_verify(&heap))
  {
    printf("i = %d\n", 0);
    abort();
  }
  for (i = 1; i < 50; i++)
  {
    periodics_ud[i].period = periodics_ud[i-1].period*PERIOD_MUL/PERIOD_DIV;
    periodics[i].time64 = gettime64() + periodics_ud[i].period;
    periodics[i].fn = periodic_fn;
    periodics[i].userdata = &periodics_ud[i];
    timer_linkheap_add(&heap, &periodics[i]);
    if (!timer_linkheap_verify(&heap))
    {
      printf("i = %zu\n", i);
      abort();
    }
  }

  for (;;)
  {
    struct timer_link *timer;
    while (gettime64() < timer_linkheap_next_expiry_time(&heap))
    {
      // spin
    }
    timer = timer_linkheap_next_expiry_timer(&heap);
    timer_linkheap_remove(&heap, timer);
    timer->fn(timer, &heap, timer->userdata);

    i = rand()%50;
    periodics[i].time64 += periodics_ud[i].period;
    timer_linkheap_modify(&heap, &periodics[i]);

    i = rand()%50;
    timer_linkheap_remove(&heap, &periodics[i]);
    timer_linkheap_add(&heap, &periodics[i]);
  }
  return 0;
}
