#include "directalloc.h"
#include <stddef.h>
#include "generalalloc.h"
#include "allocif.h"

static void *direct_allocif(struct allocif *intf, size_t sz)
{
  return direct_alloc(sz);
}

static void direct_freeif(struct allocif *intf, void *block)
{
  direct_free(block);
}

const struct allocif_ops direct_allocif_ops = {
  .alloc = direct_allocif,
  .free = direct_freeif,
};
