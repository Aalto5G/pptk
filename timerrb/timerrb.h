#ifndef _TIMERRB_H_
#define _TIMERRB_H_

#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include "rbtree.h"
#include "containerof.h"

struct timerrb {
  struct rb_tree tree;
  struct rbtimer *next;
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
  if (rb->next == NULL)
  {
    return UINT64_MAX;
  }
  return rb->next->time64;
}

int timerrb_cmp(struct rb_tree_node *n1, struct rb_tree_node *n2, void *ud);

static inline void timerrb_init(struct timerrb *rb)
{
  rb_tree_init(&rb->tree, timerrb_cmp, NULL);
}

static inline void timerrb_update_next(struct timerrb *rb)
{
  struct rb_tree_node *node;
  node = rb_tree_leftmost(&rb->tree);
  if (node == NULL)
  {
    rb->next = NULL;
  }
  else
  {
    rb->next = CONTAINER_OF(node, struct rbtimer, node);
  }
}

static inline void timerrb_add(struct timerrb *rb, struct rbtimer *timer)
{
  rb_tree_insert(&rb->tree, &timer->node);
  timerrb_update_next(rb);
}

static inline void timerrb_remove(struct timerrb *rb, struct rbtimer *timer)
{
  rb_tree_delete(&rb->tree, &timer->node);
  timerrb_update_next(rb);
}

static inline void timerrb_modify(struct timerrb *rb, struct rbtimer *timer)
{
  rb_tree_delete(&rb->tree, &timer->node);
  rb_tree_insert(&rb->tree, &timer->node);
  timerrb_update_next(rb);
}

static inline struct rbtimer *timerrb_next_expiry_timer(struct timerrb *rb)
{
  return rb->next;
}

#endif
