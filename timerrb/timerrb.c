#include "timerrb.h"

int timerrb_cmp(struct rb_tree_node *n1, struct rb_tree_node *n2, void *ud)
{
  struct rbtimer *e1 = CONTAINER_OF(n1, struct rbtimer, node);
  struct rbtimer *e2 = CONTAINER_OF(n2, struct rbtimer, node);
  if (e1->time64 < e2->time64)
  {
    return -1;
  }
  if (e1->time64 > e2->time64)
  {
    return 1;
  }
  return 0;
}
