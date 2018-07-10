#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"

static int rb_subtree_ptrs_valid(struct rb_tree_node *node)
{
  int resultl = 0, resultr = 0;
  if (node->left != NULL)
  {
    if (node->left->parent != node)
    {
      return 0;
    }
    resultl = rb_subtree_ptrs_valid(node->left);
    if (resultl == 0)
    {
      return 0;
    }
  }
  if (node->right != NULL)
  {
    if (node->right->parent != node)
    {
      return 0;
    }
    resultr = rb_subtree_ptrs_valid(node->right);
    if (resultr == 0)
    {
      return 0;
    }
  }
  return 1;
}

static int rb_subtree_height(struct rb_tree_node *node)
{
  int resultl = 0, resultr = 0;
  if (node->left != NULL)
  {
    if (node->left->parent != node)
    {
      return -1;
    }
    resultl = rb_subtree_height(node->left);
    if (resultl < 0)
    {
      return -1;
    }
  }
  if (node->right != NULL)
  {
    if (node->right->parent != node)
    {
      return -1;
    }
    resultr = rb_subtree_height(node->right);
    if (resultr < 0)
    {
      return -1;
    }
  }
  if (resultl != resultr)
  {
    return -1;
  }
  return node->is_black ? 1 + resultr : resultr;
}

static int __attribute__((unused)) rb_tree_nocmp_ptrs_valid(struct rb_tree_nocmp *tree)
{
  if (tree->root == NULL)
  {
    return 1;
  }
  if (tree->root->parent != NULL)
  {
    return 0;
  }
  if (rb_subtree_ptrs_valid(tree->root) == 0)
  {
    return 0;
  }
  return 1;
}

int rb_tree_nocmp_valid(struct rb_tree_nocmp *tree)
{
  if (tree->root == NULL)
  {
    return 1;
  }
  if (tree->root->parent != NULL)
  {
    return 0;
  }
  if (rb_subtree_height(tree->root) < 0)
  {
    return 0;
  }
  return 1;
}

struct rb_tree_node *rb_tree_nocmp_leftmost(struct rb_tree_nocmp *tree)
{
  struct rb_tree_node *node = tree->root;
  if (node == NULL)
  {
    return NULL;
  }
  for (;;)
  {
    if (node->left == NULL)
    {
      return node;
    }
    node = node->left;
  }
}

struct rb_tree_node *rb_tree_nocmp_rightmost(struct rb_tree_nocmp *tree)
{
  struct rb_tree_node *node = tree->root;
  if (node == NULL)
  {
    return NULL;
  }
  for (;;)
  {
    if (node->right == NULL)
    {
      return node;
    }
    node = node->right;
  }
}

static inline struct rb_tree_node *sibling(struct rb_tree_node *node)
{
  struct rb_tree_node *p = node->parent;
  if (p == NULL)
  {
    return NULL;
  }
  if (node == p->left)
  {
    return p->right;
  }
  else
  {
    return p->left;
  }
}

static inline struct rb_tree_node *sibling_parent(struct rb_tree_node *node, struct rb_tree_node *p)
{
  if (p == NULL)
  {
    return NULL;
  }
  if (node == p->left)
  {
    return p->right;
  }
  else
  {
    return p->left;
  }
}

static inline struct rb_tree_node *uncle(struct rb_tree_node *node)
{
  struct rb_tree_node *p = node->parent;
  struct rb_tree_node *g = p->parent;
  if (g == NULL)
  {
    return NULL;
  }
  return sibling(p);
}

static inline void rotate_left(struct rb_tree_nocmp *tree, struct rb_tree_node *p)
{
  struct rb_tree_node *parent = p->parent;
  struct rb_tree_node *q = p->right;
  struct rb_tree_node *a = p->left;
  struct rb_tree_node *b = q->left;
  struct rb_tree_node *c = q->right;
  //printf("rotating left\n");
  p->left = a;
  if (a)
  {
    a->parent = p;
  }
  p->right = b;
  if (b)
  {
    b->parent = p;
  }
  q->left = p;
  p->parent = q;
  q->right = c;
  if (c)
  {
    c->parent = q;
  }
  if (parent == NULL)
  {
    tree->root = q;
    q->parent = NULL;
    return;
  }
  if (parent->left == p)
  {
    parent->left = q;
    q->parent = parent;
  }
  else if (parent->right == p)
  {
    parent->right = q;
    q->parent = parent;
  }
  else
  {
    abort();
  }
}

static inline void rotate_right(struct rb_tree_nocmp *tree, struct rb_tree_node *q)
{
  struct rb_tree_node *parent = q->parent;
  struct rb_tree_node *p = q->left;
  struct rb_tree_node *a = p->left;
  struct rb_tree_node *b = p->right;
  struct rb_tree_node *c = q->right;
  //printf("rotating right\n");
  p->left = a;
  if (a)
  {
    a->parent = p;
  }
  p->right = q;
  q->parent = p;
  q->left = b;
  if (b)
  {
    b->parent = q;
  }
  q->right = c;
  if (c)
  {
    c->parent = q;
  }
  if (parent == NULL)
  {
    tree->root = p;
    p->parent = NULL;
    return;
  }
  if (parent->left == q)
  {
    parent->left = p;
    p->parent = parent;
  }
  else if (parent->right == q)
  {
    parent->right = p;
    p->parent = parent;
  }
  else
  {
    abort();
  }
}

void rb_tree_nocmp_insert_repair(struct rb_tree_nocmp *tree, struct rb_tree_node *node)
{
  if (node->parent == NULL)
  {
    node->is_black = 1;
  }
  else if (node->parent->is_black)
  {
    // Do nothing.
  }
  else if (uncle(node) && uncle(node)->is_black == 0)
  {
    node->parent->is_black = 1;
    uncle(node)->is_black = 1;
    node->parent->parent->is_black = 0;
    rb_tree_nocmp_insert_repair(tree, node->parent->parent); // Tail recursion
  }
  else
  {
    struct rb_tree_node *p = node->parent;
    struct rb_tree_node *g = p->parent;
    //printf("case 4 %p %p\n", p, g);
    if (g->left && node == g->left->right)
    {
      //printf("case 4.1\n");
      rotate_left(tree, p);
      node = node->left;
    }
    else if (g->right && node == g->right->left)
    {
      //printf("case 4.2\n");
      rotate_right(tree, p);
      node = node->right; 
    }
    p = node->parent;
    g = p->parent;
    //printf("case 4 cont %p %p\n", p, g);
    if (node == p->left)
    {
      //printf("case 4 step 2.1\n");
      rotate_right(tree, g);
    }
    else
    {
      //printf("case 4 step 2.2\n");
      rotate_left(tree, g);
    }
    p->is_black = 1;
    g->is_black = 0;
  }
}

void rb_tree_insert(struct rb_tree *tree, struct rb_tree_node *node)
{
  struct rb_tree_node *node2;
  node->is_black = 0;
  node->left = NULL;
  node->right = NULL;
  if (tree->nocmp.root == NULL)
  {
    tree->nocmp.root = node;
    node->parent = NULL;
    rb_tree_nocmp_insert_repair(&tree->nocmp, node);
    return;
  }
  node2 = tree->nocmp.root;
  for (;;)
  {
    if (tree->cmp(node, node2, tree->cmp_userdata) < 0)
    {
      if (node2->left == NULL)
      {
        node2->left = node;
        node->parent = node2;
        rb_tree_nocmp_insert_repair(&tree->nocmp, node);
        return;
      }
      node2 = node2->left;
    }
    else
    {
      if (node2->right == NULL)
      {
        node2->right = node;
        node->parent = node2;
        rb_tree_nocmp_insert_repair(&tree->nocmp, node);
        return;
      }
      node2 = node2->right;
    }
  }
}

static inline int is_leaf(struct rb_tree_node *node)
{
  return node == NULL;
  //return node->left == NULL && node->right == NULL;
}

static void rb_tree_delete_case6(struct rb_tree_nocmp *tree, struct rb_tree_node *n, struct rb_tree_node *parent)
{
  struct rb_tree_node *s = sibling_parent(n, parent);
 
  s->is_black = parent->is_black;
  parent->is_black = 1;
 
  if (n == parent->left)
  {
    s->right->is_black = 1;
    rotate_left(tree, parent);
  }
  else
  {
    s->left->is_black = 1;
    rotate_right(tree, parent);
  }
}

static void rb_tree_delete_case5(struct rb_tree_nocmp *tree, struct rb_tree_node *n, struct rb_tree_node *parent)
{
  struct rb_tree_node *s = sibling_parent(n, parent);
 
  if (s->is_black)
  {
    if ((n == parent->left) &&
        (s->right == NULL || s->right->is_black) &&
        (s->left != NULL && s->left->is_black == 0))
    {
      s->is_black = 0;
      s->left->is_black = 1;
      rotate_right(tree, s);
    }
    else if ((n == parent->right) &&
             (s->left == NULL || s->left->is_black) &&
             (s->right != NULL && s->right->is_black == 0))
    {
      s->is_black = 0;
      s->right->is_black = 1;
      rotate_left(tree, s);
    }
  }
  rb_tree_delete_case6(tree, n, parent);
}

static void rb_tree_delete_case4(struct rb_tree_nocmp *tree, struct rb_tree_node *n, struct rb_tree_node *parent)
{
  struct rb_tree_node *s = sibling_parent(n, parent);
  if ((parent->is_black == 0) &&
      (s == NULL || s->is_black) &&
      (s->left == NULL || s->left->is_black) &&
      (s->right == NULL || s->right->is_black))
  {
    s->is_black = 0;
    parent->is_black = 1;
  }
  else
  {
    rb_tree_delete_case5(tree, n, parent);
  }
}

static void rb_tree_delete_case1(struct rb_tree_nocmp *tree, struct rb_tree_node *node, struct rb_tree_node *parent);

static void rb_tree_delete_case3(struct rb_tree_nocmp *tree, struct rb_tree_node *n, struct rb_tree_node *parent)
{
  struct rb_tree_node *s = sibling_parent(n, parent);
  //printf("case3\n");
  if ((parent == NULL || parent->is_black) &&
      (s == NULL || s->is_black) &&
      (s->left == NULL || s->left->is_black) &&
      (s->right == NULL || s->right->is_black))
  {
    s->is_black = 0;
    rb_tree_delete_case1(tree, parent, parent->parent);
  }
  else
  {
    rb_tree_delete_case4(tree, n, parent);
  }
}

static void rb_tree_delete_case2(struct rb_tree_nocmp *tree, struct rb_tree_node *n, struct rb_tree_node *parent)
{
  struct rb_tree_node *s = sibling_parent(n, parent);
  //printf("case2\n");
  if (s && s->is_black == 0)
  {
    parent->is_black = 0;
    s->is_black = 1;
    if (n == parent->left)
    {
      rotate_left(tree, parent);
    }
    else
    {
      rotate_right(tree, parent);
    }
  }
  rb_tree_delete_case3(tree, n, parent);
}

static void rb_tree_delete_case1(struct rb_tree_nocmp *tree, struct rb_tree_node *node, struct rb_tree_node *parent)
{
  //printf("case1 %p %p\n", node, parent);
  if (parent != NULL) // XXX
  {
    rb_tree_delete_case2(tree, node, parent);
  }
}

static void __attribute__((unused)) rb_tree_exchange(struct rb_tree_nocmp *tree, struct rb_tree_node *n1, struct rb_tree_node *n2)
{
  struct rb_tree_node *n1_parent = n1->parent;
  struct rb_tree_node *n1_left = n1->left;
  struct rb_tree_node *n1_right = n1->right;
  int n1_is_black = n1->is_black;
  struct rb_tree_node *n2_parent = n2->parent;
  struct rb_tree_node *n2_left = n2->left;
  struct rb_tree_node *n2_right = n2->right;
  int n2_is_black = n2->is_black;
  if (n2_parent == n1 && n1->left == n2)
  {
    n1->left = n2_left;
    n1->right = n2_right;
    n1->parent = n2;
    if (n1->left)
    {
      n1->left->parent = n1;
    }
    if (n1->right)
    {
      n1->right->parent = n1;
    }
    n2->left = n1;
    n2->right = n1_right;
    n2->parent = n1_parent;
    if (n1_right)
    {
      n1_right->parent = n2;
    }
    if (n2->parent == NULL)
    {
      tree->root = n2;
    }
    else if (n2->parent->left == n1)
    {
      n2->parent->left = n2;
    }
    else if (n2->parent->right == n1)
    {
      n2->parent->right = n2;
    }
    else
    {
      printf("shouldn't reach\n");
      abort();
    }
    n1->is_black = n2_is_black;
    n2->is_black = n1_is_black;
    return;
  }
  if (n2_parent == n1 && n1->right == n2)
  {
    n1->left = n2_left;
    n1->right = n2_right;
    n1->parent = n2;
    if (n1->left)
    {
      n1->left->parent = n1;
    }
    if (n1->right)
    {
      n1->right->parent = n1;
    }
    n2->left = n1_left;
    n2->right = n1;
    n2->parent = n1_parent;
    if (n1_left)
    {
      n1_left->parent = n2;
    }
    if (n2->parent == NULL)
    {
      tree->root = n2;
    }
    else if (n2->parent->left == n1)
    {
      n2->parent->left = n2;
    }
    else if (n2->parent->right == n1)
    {
      n2->parent->right = n2;
    }
    else
    {
      printf("shouldn't reach\n");
      abort();
    }
    n1->is_black = n2_is_black;
    n2->is_black = n1_is_black;
    return;
  }
  if (n1_parent == n2)
  {
    rb_tree_exchange(tree, n2, n1);
    return;
  }
  //printf("exchanging\n");
#if 1
  if (n1_parent)
  {
    if (n1_parent->left == n1)
    {
      n1_parent->left = n2;
    }
    else if (n1_parent->right == n1)
    {
      n1_parent->right = n2;
    }
    else
    {
      printf("1\n");
      abort();
    }
  }
  if (n2_parent)
  {
    if (n2_parent->left == n2)
    {
      n2_parent->left = n1;
    }
    else if (n2_parent->right == n2)
    {
      n2_parent->right = n1;
    }
    else
    {
      printf("2\n");
      abort();
    }
  }
#endif
  n2->parent = n1_parent;
  n2->left = n1_left;
  n2->right = n1_right;
  n1->parent = n2_parent;
  n1->left = n2_left;
  n1->right = n2_right;
  if (n1->left)
  {
    n1->left->parent = n1;
  }
  if (n1->right)
  {
    n1->right->parent = n1;
  }
  if (n2->left)
  {
    n2->left->parent = n2;
  }
  if (n2->right)
  {
    n2->right->parent = n2;
  }
  if (n1->parent == NULL)
  {
    if (n2->parent == NULL)
    {
      abort();
    }
    tree->root = n1;
  }
  if (n2->parent == NULL)
  {
    tree->root = n2;
  }
  n1->is_black = n2_is_black;
  n2->is_black = n1_is_black;
}

static void __attribute__((unused)) rb_tree_replace(struct rb_tree_nocmp *tree, struct rb_tree_node *n1, struct rb_tree_node *n2)
{
  struct rb_tree_node *n1_parent = n1->parent;
  struct rb_tree_node *n1_left = n1->left;
  struct rb_tree_node *n1_right = n1->right;
  //printf("replacing %p\n", n1_parent);
  // substitute n2 into n1's place in the tree
  if (n1_parent)
  {
    if (n1_parent->left == n1)
    {
      n1_parent->left = n2;
    }
    else if (n1_parent->right == n1)
    {
      n1_parent->right = n2;
    }
    else
    {
      printf("1\n");
      abort();
    }
  }
  n2->parent = n1_parent;
  n2->left = n1_left;
  n2->right = n1_right;
  n1->parent = NULL;
  n1->left = NULL;
  n1->right = NULL;
  //printf("%p\n", n2->parent);
  if (n2->left)
  {
    n2->left->parent = n2;
  }
  //printf("%p\n", n2->parent);
  if (n2->right)
  {
    n2->right->parent = n2;
  }
  //printf("%p\n", n2->parent);
  if (n2->parent == NULL)
  {
    //printf("replacing root\n");
    tree->root = n2;
  }
}

static void rb_tree_delete_one_child(struct rb_tree_nocmp *tree, struct rb_tree_node *node)
{
  /*
   * Precondition: n has at most one non-leaf child.
   */
  struct rb_tree_node *child = is_leaf(node->right) ? node->left : node->right;
  //rb_tree_replace(tree, node, child);
  if (node->parent == NULL)
  {
    tree->root = child;
    if (child != NULL)
    {
      child->parent = NULL;
    }
  }
  else if (node->parent->left == node)
  {
    node->parent->left = child;
    if (child != NULL)
    {
      child->parent = node->parent;
    }
  }
  else if (node->parent->right == node)
  {
    node->parent->right = child;
    if (child != NULL)
    {
      child->parent = node->parent;
    }
  }
  if (child != NULL)
  {
    //child->is_black = node->is_black; // XXX
  }
  if (node->is_black)
  {
    if (child && child->is_black == 0)
    {
      child->is_black = 1;
    }
    else
    {
      rb_tree_delete_case1(tree, child, node->parent);
    }
  }
}

void rb_tree_nocmp_delete(struct rb_tree_nocmp *tree, struct rb_tree_node *node)
{
#if 0
  if (node->left == NULL && node->right == NULL)
  {
    if (node->parent == NULL)
    {
      tree->root = NULL;
    }
    else if (node->parent->left == node)
    {
      node->parent->left = NULL;
    }
    else if (node->parent->right == node)
    {
      node->parent->right = NULL;
    }
    else
    {
      abort();
    }
    return;
  }
#endif
  if (!is_leaf(node->left) && !is_leaf(node->right))
  {
    struct rb_tree_node *node2 = node->left;
    //struct rb_tree_node *oldright = node->right;
    for (;;)
    {
      if (node2->right == NULL)
      {
        break;
      }
      node2 = node2->right;
    }
    //printf("node->parent %p\n", node->parent);
#if 0
    if (node->parent == NULL)
    {
      tree->root = node2;
      node2->parent = NULL;
    }
    else if (node->parent->left == node)
    {
      node->parent->left = node2;
      node2->parent = node->parent;
    }
    else if (node->parent->right == node)
    {
      node->parent->right = node2;
      node2->parent = node->parent;
    }
    node2->right = node->right;
    node2->right->parent = node2;
#endif
    //abort();
    rb_tree_exchange(tree, node, node2);
    //abort();
#if 0
    if (!rb_tree_ptrs_valid(tree))
    {
      printf("invalid ptrs inside\n");
      return;
      abort();
    }
    if (!rb_tree_valid(tree))
    {
      printf("invalid 1 inside\n");
      return;
      abort();
    }
#endif
    rb_tree_delete_one_child(tree, node);
    //node2->is_black = node->is_black; // XXX
    //rb_tree_replace(tree, node, node2);
    //node2->right = oldright;
    //oldright->parent = node2;
#if 0
    if (!rb_tree_ptrs_valid(tree))
    {
      printf("invalid 2 ptrs inside\n");
      return;
      abort();
    }
    if (!rb_tree_valid(tree))
    {
      printf("invalid 2 inside\n");
      return;
      abort();
    }
#endif
    //printf("valid inside\n");
    return;
  }
  //printf("deleting one child\n");
  rb_tree_delete_one_child(tree, node);
}

