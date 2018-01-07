#include "hashseed.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

char hash_seed[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int hash_seed_inited = 0;

void hash_seed_init(void)
{
  FILE *f = fopen("/dev/urandom", "r");
  if (hash_seed_inited)
  {
    log_log(LOG_LEVEL_CRIT, "HASHSEED", "trying to reinit hash seed");
    exit(1);
  }
  if (f == NULL)
  {
    log_log(LOG_LEVEL_CRIT, "HASHSEED", "can't initialize hash seed");
    exit(1);
  }
  if (fread(hash_seed, 16, 1, f) != 1)
  {
    log_log(LOG_LEVEL_CRIT, "HASHSEED", "can't read hash seed");
    exit(1);
  }
  fclose(f);
  hash_seed_inited = 1;
}
