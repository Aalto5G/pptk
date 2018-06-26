#include "timeravl.h"
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
  struct avltimer *timer, struct timeravl *avl, void *userdata,
  void *threaddata)
{
  struct periodud *ud = userdata;
  if (!timeravl_verify(avl))
  {
    abort();
  }
  printf("periodic timer, period %g\n", ud->period/1000.0/1000.0);
  timer->time64 = gettime64() + ud->period;
  timeravl_add(avl, timer);
  if (!timeravl_verify(avl))
  {
    abort();
  }
}

int main(int argc, char **argv)
{
  struct timeravl avl;
  struct periodud periodics_ud[50];
  struct avltimer periodics[50];
  size_t i;
  timeravl_init(&avl);
  periodics_ud[0].period = MIN_PERIOD;
  periodics[0].time64 = gettime64() + MIN_PERIOD;
  periodics[0].fn = periodic_fn;
  periodics[0].userdata = &periodics_ud[0];
  timeravl_add(&avl, &periodics[0]);
  for (i = 1; i < 50; i++)
  {
    periodics_ud[i].period = periodics_ud[i-1].period*PERIOD_MUL/PERIOD_DIV;
    periodics[i].time64 = gettime64() + periodics_ud[i].period;
    periodics[i].fn = periodic_fn;
    periodics[i].userdata = &periodics_ud[i];
    timeravl_add(&avl, &periodics[i]);
  }

  for (;;)
  {
    struct avltimer *timer;
    while (gettime64() < timeravl_next_expiry_time(&avl))
    {
      // spin
    }
    timer = timeravl_next_expiry_timer(&avl);
    timeravl_remove(&avl, timer);
    timer->fn(timer, &avl, timer->userdata, NULL);

    i = rand()%50;
    periodics[i].time64 += periodics_ud[i].period;
    timeravl_modify(&avl, &periodics[i]);

    i = rand()%50;
    timeravl_remove(&avl, &periodics[i]);
    timeravl_add(&avl, &periodics[i]);
  }
  return 0;
}
