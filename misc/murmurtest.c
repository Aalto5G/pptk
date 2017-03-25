#include "murmur.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  if (murmur32(1, 2) != 3684335244)
  {
    abort();
  }
  return 0;
}
