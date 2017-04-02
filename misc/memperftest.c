#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
  char *ptr1;
  char *ptr2;
  char *ptr3;
  int i;
  struct timeval tv1, tv2;
  double diff;
  size_t sz = 1024*1024;
  while (sz <= 256*1024*1024)
  {
    ptr1 = malloc(sz);
    ptr2 = malloc(sz);
    ptr3 = malloc(sz);
    gettimeofday(&tv1, NULL);
    for (i = 0; i < 1000*1000; i++)
    {
      int i1 = rand() % (sz-1514);
      int i2 = rand() % (sz-1514);
      int i3 = rand() % (sz-1514);
      memcpy(&ptr2[i1], &ptr1[i2], 1514);
      memcpy(&ptr3[i3], &ptr2[i2], 1514);
    }
    gettimeofday(&tv2, NULL);
    diff = tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000.0/1000.0;
    printf("%zu: %g Gbps x 2\n", sz/1024/1024, 8.0*1514/diff/1000.0);
    free(ptr1);
    free(ptr2);
    free(ptr3);
    sz *= 2;
  }
  return 0;
}
