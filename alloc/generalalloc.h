#ifndef _GENERALALLOC_H_
#define _GENERALALLOC_H_

#include <stddef.h>

static inline void *alloc_allocated_block(void *obj)
{
  return ((char*)obj) - sizeof(size_t);
}

#endif
