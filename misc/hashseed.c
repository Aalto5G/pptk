#include "hashseed.h"
#include <stdio.h>
#include <stdlib.h>

char hash_seed[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int hash_seed_inited = 0;

void hash_seed_init(void)
{
  FILE *f = fopen("/dev/urandom", "r");
  if (hash_seed_inited)
  {
    printf("trying to reinit hash seed\n");
    exit(1);
  }
  if (f == NULL)
  {
    printf("can't initialize hash seed\n");
    exit(1);
  }
  if (fread(hash_seed, 16, 1, f) != 1)
  {
    printf("can't read hash seed\n");
    exit(1);
  }
  fclose(f);
  hash_seed_inited = 1;
}
