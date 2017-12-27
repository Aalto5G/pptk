#ifndef _ALLOCIF_H_
#define _ALLOCIF_H_

#include <stddef.h>

struct allocif;

struct allocif_ops {
  void *(*alloc)(struct allocif *intf, size_t sz);
  void (*free)(struct allocif *intf, void *block);
};

struct allocif {
  const struct allocif_ops *ops;
  void *userdata;
};

static inline void *allocif_alloc(struct allocif *intf, size_t sz)
{
  return intf->ops->alloc(intf, sz);
}

static inline void allocif_free(struct allocif *intf, void *block)
{
  intf->ops->free(intf, block);
}

#endif
