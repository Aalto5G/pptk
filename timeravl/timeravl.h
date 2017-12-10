#ifndef _TIMERRB_H_
#define _TIMERRB_H_

#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include "avltree.h"
#include "containerof.h"

struct timeravl {
  struct avl_tree tree;
  struct avltimer *next;
};

struct avltimer;

typedef void (*avltimer_fn)(struct avltimer *timer, struct timeravl *avl, void *userdata);

struct avltimer {
  uint64_t time64;
  avltimer_fn fn;
  void *userdata;
  struct avl_tree_node node;
};

static inline int timeravl_verify(struct timeravl *avl)
{
  return avl_tree_valid(&avl->tree);
}

static inline uint64_t timeravl_next_expiry_time(struct timeravl *avl)
{
  if (avl->next == NULL)
  {
    return UINT64_MAX;
  }
  return avl->next->time64;
}

int timeravl_cmp(struct avl_tree_node *n1, struct avl_tree_node *n2, void *ud);

static inline void timeravl_init(struct timeravl *avl)
{
  avl_tree_init(&avl->tree, timeravl_cmp, NULL);
}

static inline void timeravl_update_next(struct timeravl *avl)
{
  struct avl_tree_node *node;
  node = avl_tree_leftmost(&avl->tree);
  if (node == NULL)
  {
    avl->next = NULL;
  }
  else
  {
    avl->next = CONTAINER_OF(node, struct avltimer, node);
  }
}

static inline void timeravl_add(struct timeravl *avl, struct avltimer *timer)
{
  avl_tree_insert(&avl->tree, &timer->node);
  timeravl_update_next(avl);
}

static inline void timeravl_remove(struct timeravl *avl, struct avltimer *timer)
{
  avl_tree_delete(&avl->tree, &timer->node);
  timeravl_update_next(avl);
}

static inline void timeravl_modify(struct timeravl *avl, struct avltimer *timer)
{
  avl_tree_delete(&avl->tree, &timer->node);
  avl_tree_insert(&avl->tree, &timer->node);
  timeravl_update_next(avl);
}

static inline struct avltimer *timeravl_next_expiry_timer(struct timeravl *avl)
{
  return avl->next;
}

#endif
