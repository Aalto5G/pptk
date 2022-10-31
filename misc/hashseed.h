#ifndef _HASH_SEED_H_
#define _HASH_SEED_H_

#include <stddef.h>

extern char hash_seed[16];
extern int hash_seed_inited;

static inline void *hash_seed_get(void)
{
  if (!hash_seed_inited)
  {
    return NULL; // to crash the program
  }
  return hash_seed;
}

void hash_seed_init(void);

#endif
