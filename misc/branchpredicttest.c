#include "branchpredict.h"

int main(int argc, char **argv)
{
  int i = 0;
  for (;;)
  {
    i++;
    if (unlikely(i >= 100))
    {
      break;
    }
  }
  for (;;)
  {
    i++;
    if (likely(i < 100))
    {
      continue;
    }
    break;
  }
  return 0;
}
