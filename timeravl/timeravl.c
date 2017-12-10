#include "timeravl.h"

int timeravl_cmp(struct avl_tree_node *n1, struct avl_tree_node *n2, void *ud)
{
  struct avltimer *e1 = CONTAINER_OF(n1, struct avltimer, node);
  struct avltimer *e2 = CONTAINER_OF(n2, struct avltimer, node);
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
