#ifndef _TIMERRB_H_
#define _TIMERRB_H_

#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include "rbtree.h"
#include "containerof.h"

struct timerrb {
  struct rb_tree tree;
};

struct rbtimer;

typedef void (*rbtimer_fn)(struct rbtimer *timer, struct timerrb *rb, void *userdata);

struct rbtimer {
  uint64_t time64;
  rbtimer_fn fn;
  void *userdata;
  struct rb_tree_node node;
};

static inline int timerrb_verify(struct timerrb *rb)
{
  return rb_tree_valid(&rb->tree);
}

static inline uint64_t timerrb_next_expiry_time(struct timerrb *rb)
{
  struct rb_tree_node *node;
  if (rb->tree.root == NULL)
  {
    return UINT64_MAX;
  }
  node = rb_tree_leftmost(&rb->tree);
  return CONTAINER_OF(node, struct rbtimer, node)->time64;
}

int timerrb_cmp(struct rb_tree_node *n1, struct rb_tree_node *n2, void *ud);

static inline void timerrb_init(struct timerrb *rb)
{
  rb_tree_init(&rb->tree, timerrb_cmp, NULL);
}

static inline void timerrb_add(struct timerrb *rb, struct rbtimer *timer)
{
  rb_tree_insert(&rb->tree, &timer->node);
}

static inline void timerrb_remove(struct timerrb *rb, struct rbtimer *timer)
{
  rb_tree_delete(&rb->tree, &timer->node);
}

static inline void timerrb_modify(struct timerrb *rb, struct rbtimer *timer)
{
  timerrb_remove(rb, timer);
  timerrb_add(rb, timer);
}

static inline struct rbtimer *timerrb_next_expiry_timer(struct timerrb *rb)
{
  struct rb_tree_node *node;
  if (rb->tree.root == NULL)
  {
    return NULL;
  }
  node = rb_tree_leftmost(&rb->tree);
  return CONTAINER_OF(node, struct rbtimer, node);
}

#endif
