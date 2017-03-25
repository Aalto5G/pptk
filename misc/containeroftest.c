#include <stdlib.h>
#include "containerof.h"

struct structure {
  int a;
  int b;
  int c;
};

int main(int argc, char **argv)
{
  struct structure structure;
  int *bptr = &structure.b;
  if (CONTAINER_OF(bptr, struct structure, b) != &structure)
  {
    abort();
  }
  return 0;
}
