#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include "asalloc.h"

struct as_alloc_st st;
const int alloc_capacity = 10000;
const int alloc_native_size = 1536;

int main(int argc, char **argv)
{
  struct timeval tv1, tv2;
  size_t count = 0;
  if (as_alloc_st_init(&st, alloc_capacity, alloc_native_size) != 0)
  {
    abort();
  }
  gettimeofday(&tv1, NULL);
  for (;;)
  {
    as_free_st(&st, as_alloc_st(&st, alloc_native_size));
    count++;
    if ((count & (16*1024*1024 - 1)) == 0)
    {
      double diff;
      gettimeofday(&tv2, NULL);
      diff = tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000.0/1000.0;
      tv1 = tv2;
      printf("%g Mpps\n", 16*1024*1024/diff/1e6);
      exit(0);
    }
  }
  return 0;
}
