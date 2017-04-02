#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "siphash.h"

int main(int argc, char **argv)
{
  const unsigned char in[16] = {0};
  const unsigned char k[16] = {0};
#if 0
  int i;
#endif
  
  if (siphash_buf(k, in, sizeof(in)) != 0x32caecc280172976ULL)
  {
    abort();
  }
#if 0
  for (i = 0; i < 100*1000*1000; i++)
  {
    if (siphash_buf(k, in, sizeof(in)) == 0)
    {
      abort();
    }
  }
#endif
  return 0;
}
