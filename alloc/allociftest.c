#include "allocif.h"
#include "llalloc.h"

int main(int argc, char **argv)
{
  struct ll_alloc_st st = {};
  struct allocif intf = {.ops = &ll_allocif_ops_st, .userdata = &st};
  void *block;

  ll_alloc_st_init(&st, 1000, 4096);

  block = allocif_alloc(&intf, 1024);
  allocif_free(&intf, block);

  ll_alloc_st_free(&st);

  return 0;
}
