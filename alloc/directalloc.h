#ifndef _DIRECTALLOC_H_
#define _DIRECTALLOC_H_

#include <stddef.h>
#include "generalalloc.h"

static inline void *direct_alloc(size_t sz)
{
  char *buf = malloc(sz + sizeof(size_t));
  memcpy(buf, &sz, sizeof(size_t));
  return buf + sizeof(size_t);
}

static inline void direct_free(void *obj)
{
  if (obj == NULL)
  {
    return;
  }
  free(alloc_allocated_block(obj));
}

#endif
