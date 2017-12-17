#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "time64.h"

static inline uint32_t linux_hash(uint32_t x)
{
  return x * 0x61C88647;
}

int main(int argc, char **argv)
{
  volatile uint32_t datum = (10<<24)|(11<<16)|2;
  int i = 0;
  uint64_t start, end;
  start = gettime64();
  while (i < 1000*1000*1000)
  {
    if (linux_hash(datum) == 0)
    {
      abort();
    }
    if (linux_hash(datum) == 0)
    {
      abort();
    }
    if (linux_hash(datum) == 0)
    {
      abort();
    }
    if (linux_hash(datum) == 0)
    {
      abort();
    }
    i += 4;
  }
  end = gettime64();
  printf("%g MPPS\n", 1e9/(end-start));
  return 0;
}
