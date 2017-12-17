#include "timerrb.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "time64.h"

#define MIN_PERIOD (1000*1000)
#define PERIOD_MUL 5
#define PERIOD_DIV 4

struct periodud {
  uint64_t period;
};

static void periodic_fn(
  struct rbtimer *timer, struct timerrb *rb, void *userdata)
{
  struct periodud *ud = userdata;
  if (!timerrb_verify(rb))
  {
    abort();
  }
  printf("periodic timer, period %g\n", ud->period/1000.0/1000.0);
  timer->time64 = gettime64() + ud->period;
  timerrb_add(rb, timer);
  if (!timerrb_verify(rb))
  {
    abort();
  }
}

int main(int argc, char **argv)
{
  struct timerrb rb;
  struct periodud periodics_ud[50];
  struct rbtimer periodics[50];
  size_t i;
  timerrb_init(&rb);
  periodics_ud[0].period = MIN_PERIOD;
  periodics[0].time64 = gettime64() + MIN_PERIOD;
  periodics[0].fn = periodic_fn;
  periodics[0].userdata = &periodics_ud[0];
  timerrb_add(&rb, &periodics[0]);
  for (i = 1; i < 50; i++)
  {
    periodics_ud[i].period = periodics_ud[i-1].period*PERIOD_MUL/PERIOD_DIV;
    periodics[i].time64 = gettime64() + periodics_ud[i].period;
    periodics[i].fn = periodic_fn;
    periodics[i].userdata = &periodics_ud[i];
    timerrb_add(&rb, &periodics[i]);
  }

  for (;;)
  {
    struct rbtimer *timer;
    while (gettime64() < timerrb_next_expiry_time(&rb))
    {
      // spin
    }
    timer = timerrb_next_expiry_timer(&rb);
    timerrb_remove(&rb, timer);
    timer->fn(timer, &rb, timer->userdata);

    i = rand()%50;
    periodics[i].time64 += periodics_ud[i].period;
    timerrb_modify(&rb, &periodics[i]);

    i = rand()%50;
    timerrb_remove(&rb, &periodics[i]);
    timerrb_add(&rb, &periodics[i]);
  }
  return 0;
}
