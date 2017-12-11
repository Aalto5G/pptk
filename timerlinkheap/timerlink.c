#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include "timerlink.h"

static inline size_t timer_linkheap_parent(size_t i)
{
  return (i-1)/2;
}

static inline size_t timer_linkheap_first_child(size_t i)
{
  return 2*i+1;
}

static struct timer_link *timer_linkheap_nth(
  struct timer_linkheap *heap, size_t n)
{
  char path[64] = {0};
  int pathcnt = 0;
  int i;
  struct timer_link *link = NULL;
  while (n != 0)
  {
    if ((n % 2) == 0)
    {
      path[pathcnt++] = 1;
    }
    else
    {
      path[pathcnt++] = 0;
    }
    n = timer_linkheap_parent(n);
  }
  link = heap->root;
  for (i = pathcnt - 1; i >= 0; i--)
  {
    if (path[i])
    {
      link = link->right;
    }
    else
    {
      link = link->left;
    }
  }
  return link;
}

static inline struct timer_link *timer_linkheap_last(
  struct timer_linkheap *heap)
{
  return timer_linkheap_nth(heap, heap->size - 1);
}

static inline struct timer_link *timer_linkheap_last_plus_one_parent(
  struct timer_linkheap *heap)
{
  return timer_linkheap_nth(heap, timer_linkheap_parent(heap->size));
}

static inline void timer_linkheap_swap_parentchild(
  struct timer_linkheap *heap, struct timer_link *parent, struct timer_link *child)
{
  if (child->parent != parent)
  {
    abort();
  }
  if (parent->left == child)
  {
    struct timer_link *parent_right = parent->right;
    struct timer_link *parent_parent = parent->parent;
    struct timer_link *child_left = child->left;
    struct timer_link *child_right = child->right;
    child->parent = parent_parent;
    child->left = parent;
    child->right = parent_right;
    parent->left = child_left;
    parent->right = child_right;
    parent->parent = child;
    if (parent->left)
    {
      parent->left->parent = parent;
    }
    if (parent->right)
    {
      parent->right->parent = parent;
    }
    if (child->right)
    {
      child->right->parent = child;
    }
    if (child->parent == NULL)
    {
      heap->root = child;
    }
    else if (child->parent->left == parent)
    {
      child->parent->left = child;
    }
    else if (child->parent->right == parent)
    {
      child->parent->right = child;
    }
    else
    {
      abort();
    }
  }
  else if (parent->right == child)
  {
    struct timer_link *parent_left = parent->left;
    struct timer_link *parent_parent = parent->parent;
    struct timer_link *child_left = child->left;
    struct timer_link *child_right = child->right;
    child->parent = parent_parent;
    child->left = parent_left;
    child->right = parent;
    parent->left = child_left;
    parent->right = child_right;
    parent->parent = child;
    if (parent->left)
    {
      parent->left->parent = parent;
    }
    if (parent->right)
    {
      parent->right->parent = parent;
    }
    if (child->left)
    {
      child->left->parent = child;
    }
    if (child->parent == NULL)
    {
      heap->root = child;
    }
    else if (child->parent->left == parent)
    {
      child->parent->left = child;
    }
    else if (child->parent->right == parent)
    {
      child->parent->right = child;
    }
    else
    {
      abort();
    }
  }
  else
  {
    abort();
  }
}

static inline void timer_linkheap_swap(
  struct timer_linkheap *heap, struct timer_link *x1, struct timer_link *x2)
{
  struct timer_link *left1 = NULL;
  struct timer_link *right1 = NULL;
  struct timer_link *parent1 = NULL;
  struct timer_link *left2 = NULL;
  struct timer_link *right2 = NULL;
  struct timer_link *parent2 = NULL;
  if (x1->parent == x2)
  {
    timer_linkheap_swap_parentchild(heap, x2, x1);
    return;
  }
  if (x2->parent == x1)
  {
    timer_linkheap_swap_parentchild(heap, x1, x2);
    return;
  }
  left1 = x1->left;
  right1 = x1->right;
  parent1 = x1->parent;
  left2 = x2->left;
  right2 = x2->right;
  parent2 = x2->parent;
  x1->left = left2;
  x1->right = right2;
  x1->parent = parent2;
  x2->left = left1;
  x2->right = right1;
  x2->parent = parent1;
  if (x2->parent == NULL)
  {
    heap->root = x2;
  }
  else if (x2->parent->left == x1)
  {
    x2->parent->left = x2;
  }
  else if (x2->parent->right == x1)
  {
    x2->parent->right = x2;
  }
  else
  {
    abort();
  }
  if (x1->parent == NULL)
  {
    heap->root = x1;
  }
  else if (x1->parent->left == x2)
  {
    x1->parent->left = x1;
  }
  else if (x1->parent->right == x2)
  {
    x1->parent->right = x1;
  }
  else
  {
    abort();
  }
  if (x1->left != NULL)
  {
    x1->left->parent = x1;
  }
  if (x1->right != NULL)
  {
    x1->right->parent = x1;
  }
  if (x2->left != NULL)
  {
    x2->left->parent = x2;
  }
  if (x2->right != NULL)
  {
    x2->right->parent = x2;
  }
}

void timer_linkheap_siftdown(
  struct timer_linkheap *heap, struct timer_link *link)
{
  for (;;)
  {
    struct timer_link *fc = link->left;
    struct timer_link *sc = link->right;
    if (fc == NULL)
    {
      break;
    }
    else if (sc == NULL)
    {
      if (fc->time64 < link->time64)
      {
        timer_linkheap_swap_parentchild(heap, link, fc);
        //link = fc;
      }
      else
      {
        break;
      }
    }
    else
    {
      struct timer_link *minlink;
      if (fc->time64 < sc->time64)
      {
        minlink = fc;
      }
      else
      {
        minlink = sc;
      }
      if (minlink->time64 < link->time64)
      {
        timer_linkheap_swap_parentchild(heap, link, minlink);
        //link = minlink;
      }
      else
      {
        break;
      }
    }
  }
}

void timer_linkheap_siftup(
  struct timer_linkheap *heap, struct timer_link *link)
{
  while (link->parent != NULL)
  {
    struct timer_link *parent = link->parent;
    if (parent->time64 > link->time64)
    {
      timer_linkheap_swap_parentchild(heap, parent, link);
      //link = parent;
    }
    else
    {
      break;
    }
  }
}

static int timer_linkheap_verify_tree(struct timer_linkheap *heap,
                                      struct timer_link *link)
{
  if (link->parent != NULL)
  {
    if (link->time64 < link->parent->time64)
    {
      return 0;
    }
  }
  if (link->left != NULL)
  {
    if (link->left->parent != link)
    {
      return 0;
    }
    if (timer_linkheap_verify_tree(heap, link->left) == 0)
    {
      return 0;
    }
  }
  if (link->right != NULL)
  {
    if (link->left == NULL)
    {
      return 0;
    }
    if (link->right->parent != link)
    {
      return 0;
    }
    if (timer_linkheap_verify_tree(heap, link->right) == 0)
    {
      return 0;
    }
  }
  return 1;
}

int timer_linkheap_verify(struct timer_linkheap *heap)
{
  if (heap->root && !timer_linkheap_verify_tree(heap, heap->root))
  {
    return 0;
  }
  return 1;
}

void timer_linkheap_remove(struct timer_linkheap *heap, struct timer_link *timer)
{
  struct timer_link *parent = NULL;
  struct timer_link *last = timer_linkheap_last(heap);
  if (last == NULL)
  {
    abort();
  }
  if (timer == last)
  {
    if (timer->parent != NULL)
    {
      if (timer->parent->left == timer)
      {
        timer->parent->left = NULL;
      }
      else if (timer->parent->right == timer)
      {
        timer->parent->right = NULL;
      }
      else
      {
        abort();
      }
    }
    else
    {
      heap->root = NULL;
    }
    heap->size--;
    return;
  }
  timer_linkheap_swap(heap, timer, last);
  if (timer->parent->left == timer)
  {
    timer->parent->left = NULL;
  }
  else if (timer->parent->right == timer)
  {
    timer->parent->right = NULL;
  }
  else
  {
    abort();
  }
  heap->size--;
  if (last->parent == NULL)
  {
    timer_linkheap_siftdown(heap, last);
    return;
  }
  parent = last->parent;
  if (last->time64 < parent->time64)
  {
    timer_linkheap_siftup(heap, last);
  }
  else
  {
    timer_linkheap_siftdown(heap, last);
  }
}

void timer_linkheap_modify(struct timer_linkheap *heap, struct timer_link *timer)
{
  struct timer_link *parent;
  if (timer->parent == NULL)
  {
    timer_linkheap_siftdown(heap, timer);
    return;
  }
  parent = timer->parent;
  if (timer->time64 < parent->time64)
  {
    timer_linkheap_siftup(heap, timer);
  }
  else
  {
    timer_linkheap_siftdown(heap, timer);
  }
}

void timer_linkheap_add(struct timer_linkheap *heap, struct timer_link *timer)
{
  struct timer_link *new_parent;
  timer->left = NULL;
  timer->right = NULL;
  if (heap->root == NULL)
  {
    heap->size++;
    timer->parent = NULL;
    heap->root = timer;
    return;
  }
  new_parent = timer_linkheap_last_plus_one_parent(heap);
  timer->parent = new_parent;
  if (new_parent->left == NULL)
  {
    new_parent->left = timer;
    if (new_parent->right != NULL)
    {
      abort();
    }
  }
  else if (new_parent->right == NULL)
  {
    new_parent->right = timer;
  }
  else
  {
    abort();
  }
  heap->size++;
  timer_linkheap_siftup(heap, timer);
}
