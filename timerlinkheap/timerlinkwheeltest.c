#include "timerlinkwheel.h"
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
  struct timer_link *timer, struct timer_linkheap *isnull, void *userdata, void *threaddata)
{
  struct timer_linkwheel_threaddata *td = threaddata;
  struct periodud *ud = userdata;
  printf("periodic timer, period %g\n", ud->period/1000.0/1000.0);
  timer->time64 = gettime64() + ud->period;
  timer_linkwheel_add(
    td->linkwheel, CONTAINER_OF(timer, struct timer_linkwheel_task, link));
}

int main(int argc, char **argv)
{
  struct timer_linkwheel wheel;
  struct periodud periodics_ud[50];
  struct timer_linkwheel_task periodics[50];
  size_t i;
  timer_linkwheel_init(&wheel, 100*1000, 512, gettime64(), 0);
  periodics_ud[0].period = MIN_PERIOD;
  periodics[0].link.time64 = gettime64() + MIN_PERIOD;
  periodics[0].link.fn = periodic_fn;
  periodics[0].link.userdata = &periodics_ud[0];
  timer_linkwheel_add(&wheel, &periodics[0]);
  for (i = 1; i < 50; i++)
  {
    periodics_ud[i].period = periodics_ud[i-1].period*PERIOD_MUL/PERIOD_DIV;
    periodics[i].link.time64 = gettime64() + periodics_ud[i].period;
    periodics[i].link.fn = periodic_fn;
    periodics[i].link.userdata = &periodics_ud[i];
    timer_linkwheel_add(&wheel, &periodics[i]);
  }

  for (;;)
  {
    while (gettime64() < timer_linkwheel_next_expiry_time(&wheel))
    {
      // spin
    }
    timer_linkwheel_process(&wheel, gettime64(), NULL);

    i = rand()%50;
    periodics[i].link.time64 += periodics_ud[i].period;
    timer_linkwheel_modify(&wheel, &periodics[i]);

    i = rand()%50;
    timer_linkwheel_remove(&wheel, &periodics[i]);
    timer_linkwheel_add(&wheel, &periodics[i]);
  }
  return 0;
}
