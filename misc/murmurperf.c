#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "murmur.h"

static inline uint64_t gettime64(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000UL*1000UL + tv.tv_usec;
}

static inline uint32_t murmur_hash(uint32_t x)
{
  return murmur32(0x12345678U, x);
}

int main(int argc, char **argv)
{
  volatile uint32_t datum = (10<<24)|(11<<16)|2;
  int i = 0;
  uint64_t start, end;
  start = gettime64();
  while (i < 1000*1000*1000)
  {
    if (murmur_hash(datum) == 0)
    {
      abort();
    }
    if (murmur_hash(datum) == 0)
    {
      abort();
    }
    if (murmur_hash(datum) == 0)
    {
      abort();
    }
    if (murmur_hash(datum) == 0)
    {
      abort();
    }
    i += 4;
  }
  end = gettime64();
  printf("%g MPPS\n", 1e9/(end-start));
  return 0;
}
