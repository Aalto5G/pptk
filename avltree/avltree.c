#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "avltree.h"

static int avl_subtree_ptrs_valid(struct avl_tree_node *node)
{
  int resultl = 0, resultr = 0;
  if (node->left != NULL)
  {
    if (node->left->parent != node)
    {
      return 0;
    }
    resultl = avl_subtree_ptrs_valid(node->left);
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
    resultr = avl_subtree_ptrs_valid(node->right);
    if (resultr == 0)
    {
      return 0;
    }
  }
  return 1;
}

static int avl_subtree_height(struct avl_tree_node *node)
{
  int resultl = 0, resultr = 0;
  if (node->left != NULL)
  {
    if (node->left->parent != node)
    {
      return -1;
    }
    resultl = avl_subtree_height(node->left);
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
    resultr = avl_subtree_height(node->right);
    if (resultr < 0)
    {
      return -1;
    }
  }
  if (node->balance < -1 || node->balance > 1)
  {
    return -1;
  }
  if (resultl < resultr - 1 || resultl - 1 > resultr)
  {
    return -1;
  }
  if (resultr - resultl != node->balance)
  {
    return -1;
  }
  if (resultr > resultl)
  {
    return 1 + resultr;
  }
  else
  {
    return 1 + resultl;
  }
}

static int __attribute__((unused)) avl_tree_ptrs_valid(struct avl_tree *tree)
{
  if (tree->root == NULL)
  {
    return 1;
  }
  if (tree->root->parent != NULL)
  {
    return 0;
  }
  if (avl_subtree_ptrs_valid(tree->root) == 0)
  {
    return 0;
  }
  return 1;
}

int avl_tree_valid(struct avl_tree *tree)
{
  if (tree->root == NULL)
  {
    return 1;
  }
  if (tree->root->parent != NULL)
  {
    return 0;
  }
  if (avl_subtree_height(tree->root) < 0)
  {
    return 0;
  }
  return 1;
}

struct avl_tree_node *avl_tree_leftmost(struct avl_tree *tree)
{
  struct avl_tree_node *node = tree->root;
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

struct avl_tree_node *avl_tree_rightmost(struct avl_tree *tree)
{
  struct avl_tree_node *node = tree->root;
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

#if 0
static inline struct avl_tree_node *sibling(struct avl_tree_node *node)
{
  struct avl_tree_node *p = node->parent;
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

static inline struct avl_tree_node *sibling_parent(struct avl_tree_node *node, struct avl_tree_node *p)
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

static inline struct avl_tree_node *uncle(struct avl_tree_node *node)
{
  struct avl_tree_node *p = node->parent;
  struct avl_tree_node *g = p->parent;
  if (g == NULL)
  {
    return NULL;
  }
  return sibling(p);
}
#endif

static struct avl_tree_node *
rotate_left(struct avl_tree_node *X, struct avl_tree_node *Z)
{
    struct avl_tree_node *t23;
    t23 = Z->left;
    X->right = t23;
    if (t23 != NULL)
    {
        t23->parent = X;
    }
    Z->left = X;
    X->parent = Z;
    if (Z->balance == 0)
    {
        X->balance = +1;
        Z->balance = -1;
    }
    else
    {
        X->balance = 0;
        Z->balance = 0;
    }
    return Z;
}

static struct avl_tree_node *
rotate_right(struct avl_tree_node *X, struct avl_tree_node *Z)
{
    struct avl_tree_node *t23;
    t23 = Z->right;
    X->left = t23;
    if (t23 != NULL)
    {
        t23->parent = X;
    }
    Z->right = X;
    X->parent = Z;
    if (Z->balance == 0)
    {
        X->balance = -1;
        Z->balance = +1;
    }
    else
    {
        X->balance = 0;
        Z->balance = 0;
    }
    return Z;
}

static struct avl_tree_node *
rotate_rightleft(struct avl_tree_node *X, struct avl_tree_node *Z)
{
    struct avl_tree_node *Y = Z->left;
    struct avl_tree_node *t3 = Y->right;
    struct avl_tree_node *t2;
    Z->left = t3;
    if (t3 != NULL)
    {
        t3->parent = Z;
    }
    Y->right = Z;
    Z->parent = Y;
    t2 = Y->left;
    X->right = t2;
    if (t2 != NULL)
    {
        t2->parent = X;
    }
    Y->left = X;
    X->parent = Y;
    if (Y->balance > 0)
    {
        X->balance = -1;
        Z->balance = 0;
    }
    else
    {
        if (Y->balance == 0)
        {
            X->balance = 0;
            Z->balance = 0;
        }
        else
        {
            X->balance = 0;
            Z->balance = +1;
        }
    }
    Y->balance = 0;
    return Y;
}

static struct avl_tree_node *
rotate_leftright(struct avl_tree_node *X, struct avl_tree_node *Z)
{
    struct avl_tree_node *Y = Z->right;
    struct avl_tree_node *t3 = Y->left;
    struct avl_tree_node *t2;
    Z->right = t3;
    if (t3 != NULL)
    {
        t3->parent = Z;
    }
    Y->left = Z;
    Z->parent = Y;
    t2 = Y->right;
    X->left = t2;
    if (t2 != NULL)
    {
        t2->parent = X;
    }
    Y->right = X;
    X->parent = Y;
    if (Y->balance < 0)
    {
        X->balance = +1;
        Z->balance = 0;
    }
    else
    {
        if (Y->balance == 0)
        {
            X->balance = 0;
            Z->balance = 0;
        }
        else
        {
            X->balance = 0;
            Z->balance = -1;
        }
    }
    Y->balance = 0;
    return Y;
}

static void
avl_tree_insert_repair(struct avl_tree *tree, struct avl_tree_node *Z)
{
  struct avl_tree_node *X = NULL, *G = NULL, *N = NULL;

  for (X = Z->parent; X != NULL; X = Z->parent)
  {
    if (Z == X->right)
    {
      if (X->balance > 0)
      {
        G = X->parent;
        if (Z->balance < 0)
        {
          N = rotate_rightleft(X, Z);
        }
        else
        {
          N = rotate_left(X, Z);
        }
      }
      else
      {
        if (X->balance < 0)
        {
          X->balance = 0;
          break;
        }
        X->balance = +1;
        Z = X;
        continue;
      }
    }
    else
    {
      if (X->balance < 0)
      {
        G = X->parent;
        if (Z->balance > 0)
        {
          N = rotate_leftright(X, Z);
        }
        else
        {
          N = rotate_right(X, Z);
        }
      }
      else
      {
        if (X->balance > 0)
        {
          X->balance = 0;
          break;
        }
        X->balance = -1;
        Z = X;
        continue;
      }
    }
    N->parent = G;
    if (G != NULL)
    {
      if (X == G->left)
      {
        G->left = N;
      }
      else
      {
        G->right = N;
      }
      break;
    }
    else
    {
      tree->root = N;
      break;
    }
    break;
  }
}

void avl_tree_insert(struct avl_tree *tree, struct avl_tree_node *node)
{
  struct avl_tree_node *node2;
  node->balance = 0;
  node->left = NULL;
  node->right = NULL;
  if (tree->root == NULL)
  {
    tree->root = node;
    node->parent = NULL;
    return;
  }
  node2 = tree->root;
  for (;;)
  {
    if (tree->cmp(node, node2, tree->cmp_userdata) < 0)
    {
      if (node2->left == NULL)
      {
        node2->left = node;
        node->parent = node2;
        avl_tree_insert_repair(tree, node);
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
        avl_tree_insert_repair(tree, node);
        return;
      }
      node2 = node2->right;
    }
  }
}

static inline int is_leaf(struct avl_tree_node *node)
{
  return node == NULL;
}

static void avl_tree_exchange(
  struct avl_tree *tree, struct avl_tree_node *n1, struct avl_tree_node *n2)
{
  struct avl_tree_node *n1_parent = n1->parent;
  struct avl_tree_node *n1_left = n1->left;
  struct avl_tree_node *n1_right = n1->right;
  int n1_balance = n1->balance;
  struct avl_tree_node *n2_parent = n2->parent;
  struct avl_tree_node *n2_left = n2->left;
  struct avl_tree_node *n2_right = n2->right;
  int n2_balance = n2->balance;
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
    n1->balance = n2_balance;
    n2->balance = n1_balance;
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
    n1->balance = n2_balance;
    n2->balance = n1_balance;
    return;
  }
  if (n1_parent == n2)
  {
    avl_tree_exchange(tree, n2, n1);
    return;
  }
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
  n1->balance = n2_balance;
  n2->balance = n1_balance;
}

static void
avl_tree_delete_repair(struct avl_tree *tree, struct avl_tree_node *N)
{
  struct avl_tree_node *X = NULL, *G = NULL, *Z = NULL;
  int b = 0;
  for (X = N->parent; X != NULL; X = G)
  {
    G = X->parent;
    if (N == X->left)
    {
      if (X->balance > 0)
      {
        Z = X->right;
        b = Z->balance;
        if (b < 0)
        {
          N = rotate_rightleft(X, Z);
        }
        else
        {
          N = rotate_left(X, Z);
        }
      }
      else
      {
        if (X->balance == 0)
        {
          X->balance = +1;
          break;
        }
        N = X;
        N->balance = 0;
        continue;
      }
    }
    else
    {
      if (X->balance < 0)
      {
        Z = X->left;
        b = Z->balance;
        if (b > 0)
        {
          N = rotate_leftright(X, Z);
        }
        else
        {
          N = rotate_right(X, Z);
        }
      }
      else
      {
        if (X->balance == 0)
        {
          X->balance = -1;
          break;
        }
        N = X;
        N->balance = 0;
        continue;
      }
    }
    N->parent = G;
    if (G != NULL)
    {
      if (X == G->left)
      {
        G->left = N;
      }
      else
      {
        G->right = N;
      }
      if (b == 0)
      {
        break;
      }
    }
    else
    {
      tree->root = N;
    }
  }
}

static void
avl_tree_delete_one_child(struct avl_tree *tree, struct avl_tree_node *node)
{
  /*
   * Precondition: n has at most one non-leaf child.
   */
  struct avl_tree_node *child = is_leaf(node->right) ? node->left : node->right;
  avl_tree_delete_repair(tree, node);
  child = is_leaf(node->right) ? node->left : node->right;
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
}

void avl_tree_delete(struct avl_tree *tree, struct avl_tree_node *node)
{
  if (!is_leaf(node->left) && !is_leaf(node->right))
  {
    struct avl_tree_node *node2 = node->right;
    for (;;)
    {
      if (node2->left == NULL)
      {
        break;
      }
      node2 = node2->left;
    }
    //printf("node->parent %p\n", node->parent);
    //printf("exchange\n");
    avl_tree_exchange(tree, node, node2);
    //print_tree(tree);
#if 0
    if (!avl_tree_ptrs_valid(tree))
    {
      printf("invalid ptrs inside\n");
      return;
      abort();
    }
    if (!avl_tree_valid(tree))
    {
      printf("invalid 1 inside\n");
      return;
      abort();
    }
#endif
    avl_tree_delete_one_child(tree, node);
#if 0
    if (!avl_tree_ptrs_valid(tree))
    {
      printf("invalid 2 ptrs inside\n");
      return;
      abort();
    }
    if (!avl_tree_valid(tree))
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
  avl_tree_delete_one_child(tree, node);
}

