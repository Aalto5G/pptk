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

static int __attribute__((unused)) avl_subtree_fast_height(struct avl_tree_node *node)
{
  int resultl = 0;
  int resultr = 0;
  if (node == NULL)
  {
    return 0;
  }
  if (node->left != NULL)
  {
    resultl = avl_subtree_fast_height(node->left);
  }
  resultr = resultl + node->balance;
  if (resultr > resultl)
  {
    return 1 + resultr;
  }
  else
  {
    return 1 + resultl;
  }
}

static void __attribute__((unused)) avl_calculate_balance(struct avl_tree_node *node)
{
  int resultl = 0;
  int resultr = 0;
  if (node == NULL)
  {
    return;
  }
  if (node->left != NULL)
  {
    resultl = avl_subtree_fast_height(node->left);
  }
  resultr = avl_subtree_fast_height(node->right);
  node->balance = resultr - resultl;
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

#if 0
static inline void rotate_left(struct avl_tree *tree, struct avl_tree_node *p)
{
  struct avl_tree_node *parent = p->parent;
  struct avl_tree_node *q = p->right;
  struct avl_tree_node *a = p->left;
  struct avl_tree_node *b = q->left;
  struct avl_tree_node *c = q->right;
#if 1
  int height_a, height_b, height_c;
  int max_ab;
  height_a = avl_subtree_fast_height(a);
  height_b = avl_subtree_fast_height(b);
  height_c = avl_subtree_fast_height(c);
  if (height_a > height_b)
  {
    max_ab = height_a;
  }
  else
  {
    max_ab = height_b;
  }
  q->balance = height_c - max_ab - 1;
  p->balance = height_c - height_b;
#endif
  printf("left\n");
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
#endif

#if 0
static inline void rotate_right(struct avl_tree *tree, struct avl_tree_node *q)
{
  struct avl_tree_node *parent = q->parent;
  struct avl_tree_node *p = q->left;
  struct avl_tree_node *a = p->left;
  struct avl_tree_node *b = p->right;
  struct avl_tree_node *c = q->right;
#if 1
  int height_a, height_b, height_c;
  int max_bc;
  height_a = avl_subtree_fast_height(a);
  height_b = avl_subtree_fast_height(b);
  height_c = avl_subtree_fast_height(c);
  if (height_b > height_c)
  {
    max_bc = height_b;
  }
  else
  {
    max_bc = height_c;
  }
  q->balance = height_c - height_b;
  p->balance = max_bc + 1 - height_a;
#endif
  printf("right\n");
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
#endif

#if 0
static inline void rotate_rightleft(struct avl_tree *tree, struct avl_tree_node *p)
{
  //struct avl_tree_node *q = p->parent;
  printf("right, left\n");
  rotate_right(tree, p);
  rotate_left(tree, p->parent);
}

static inline void rotate_leftright(struct avl_tree *tree, struct avl_tree_node *p)
{
  //struct avl_tree_node *q = p->parent;
  printf("left, right\n");
  rotate_left(tree, p);
  rotate_right(tree, p->parent);
}
#endif

static struct avl_tree_node *rotate_left(struct avl_tree_node *X, struct avl_tree_node *Z) {
    struct avl_tree_node *t23;
    // Z is by 2 higher than its sibling
    t23 = Z->left; // Inner child of Z
    X->right = t23;
    //printf("L\n");
    if (t23 != NULL)
        t23->parent = X;
    Z->left = X;
    X->parent = Z;
    // 1st case, BalanceFactor(Z) == 0, only happens with deletion, not insertion:
    if (Z->balance == 0) { // t23 has been of same height as t4
        X->balance = +1;   // t23 now higher
        Z->balance = -1;   // t4 now lower than X
    } else { // 2nd case happens with insertion or deletion:
        X->balance = 0;
        Z->balance = 0;
    }
    return Z; // return new root of rotated subtree
}

static struct avl_tree_node *rotate_right(struct avl_tree_node *X, struct avl_tree_node *Z) {
    struct avl_tree_node *t23;
    // Z is by 2 higher than its sibling
    t23 = Z->right; // Inner child of Z
    X->left = t23;
    //printf("R\n");
    if (t23 != NULL)
        t23->parent = X;
    Z->right = X;
    X->parent = Z;
    // 1st case, BalanceFactor(Z) == 0, only happens with deletion, not insertion:
    if (Z->balance == 0) { // t23 has been of same height as t4
        X->balance = -1;   // t23 now higher
        Z->balance = +1;   // t4 now lower than X
    } else { // 2nd case happens with insertion or deletion:
        X->balance = 0;
        Z->balance = 0;
    }
    return Z; // return new root of rotated subtree
}

static struct avl_tree_node *rotate_rightleft(struct avl_tree_node *X, struct avl_tree_node *Z) {
    // Z is by 2 higher than its sibling
    struct avl_tree_node *Y = Z->left; // Inner child of Z
    // Y is by 1 higher than sibling
    struct avl_tree_node *t3 = Y->right;
    //printf("RL\n");
    Z->left = t3;
    if (t3 != NULL)
        t3->parent = Z;
    Y->right = Z;
    Z->parent = Y;
    struct avl_tree_node *t2 = Y->left;
    X->right = t2;
    if (t2 != NULL)
        t2->parent = X;
    Y->left = X;
    X->parent = Y;
    // 1st case, Y->balance > 0, happens with insertion or deletion:
    if (Y->balance > 0) { // t3 was higher
        X->balance = -1;  // t1 now higher
        Z->balance = 0;
    } else // 2nd case, Y->balance == 0, only happens with deletion, not insertion:
        if (Y->balance == 0) {
            X->balance = 0;
            Z->balance = 0;
        } else { // 3rd case happens with insertion or deletion:
            // t2 was higher
            X->balance = 0;
            Z->balance = +1;  // t4 now higher
        }
    Y->balance = 0;
    return Y; // return new root of rotated subtree
}

static struct avl_tree_node *rotate_leftright(struct avl_tree_node *X, struct avl_tree_node *Z) {
    // Z is by 2 higher than its sibling
    struct avl_tree_node *Y = Z->right; // Inner child of Z
    // Y is by 1 higher than sibling
    struct avl_tree_node *t3 = Y->left;
    //printf("LR\n");
    Z->right = t3;
    if (t3 != NULL)
        t3->parent = Z;
    Y->left = Z;
    Z->parent = Y;
    struct avl_tree_node *t2 = Y->right;
    X->left = t2;
    if (t2 != NULL)
        t2->parent = X;
    Y->right = X;
    X->parent = Y;
    // 1st case, Y->balance > 0, happens with insertion or deletion:
    if (Y->balance < 0) { // t3 was higher
        X->balance = +1;  // t1 now higher
        Z->balance = 0;
    } else // 2nd case, Y->balance == 0, only happens with deletion, not insertion:
        if (Y->balance == 0) {
            X->balance = 0;
            Z->balance = 0;
        } else { // 3rd case happens with insertion or deletion:
            // t2 was higher
            X->balance = 0;
            Z->balance = -1;  // t4 now higher
        }
    Y->balance = 0;
    return Y; // return new root of rotated subtree
}

static void __attribute__((unused)) avl_tree_insert_repair(struct avl_tree *tree, struct avl_tree_node *Z)
{
  struct avl_tree_node *X = NULL, *G = NULL, *N = NULL;
  for (X = Z->parent; X != NULL; X = Z->parent) { // Loop (possibly up to the root)
      // BalanceFactor(X) has to be updated:
      if (Z == X->right) { // The right subtree increases
          if (X->balance > 0) { // X is right-heavy
              // ===> the temporary BalanceFactor(X) == +2
              // ===> rebalancing is required.
              G = X->parent; // Save parent of X around rotations
              if (Z->balance < 0)      // Right Left Case     (see figure 5)
                  N = rotate_rightleft(X, Z); // Double rotation: Right(Z) then Left(X)
              else                           // Right Right Case    (see figure 4)
                  N = rotate_left(X, Z);     // Single rotation Left(X)
              // After rotation adapt parent link
          } else {
              if (X->balance < 0) {
                  X->balance = 0; // Z’s height increase is absorbed at X.
                  break; // Leave the loop
              }
              X->balance = +1;
              Z = X; // Height(Z) increases by 1
              continue;
          }
      } else { // Z == left_child(X): the left subtree increases
          if (X->balance < 0) { // X is left-heavy
              // ===> the temporary BalanceFactor(X) == –2
              // ===> rebalancing is required.
              G = X->parent; // Save parent of X around rotations
              if (Z->balance > 0)      // Left Right Case
                  N = rotate_leftright(X, Z); // Double rotation: Left(Z) then Right(X)
              else                           // Left Left Case
                  N = rotate_right(X, Z);    // Single rotation Right(X)
              // After rotation adapt parent link
          } else {
              if (X->balance > 0) {
                  X->balance = 0; // Z’s height increase is absorbed at X.
                  break; // Leave the loop
              }
              X->balance = -1;
              Z = X; // Height(Z) increases by 1
              continue;
          }
      }
      // After a rotation adapt parent link:
      // N is the new root of the rotated subtree
      // Height does not change: Height(N) == old Height(X)
#if 1
      N->parent = G;
      if (G != NULL) {
          if (X == G->left)
              G->left = N;
          else
              G->right = N;
          break;
      } else {
          tree->root = N; // N is the new root of the total tree
          break;
      }
#endif
      break;
      // There is no fall thru, only break; or continue;
  }
  // Unless loop is left via break, the height of the total tree increases by 1.
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
        //node2->balance--;
        node2->left = node;
        node->parent = node2;
        node2 = node2->parent;
        while (node2 != NULL)
        {
          //avl_calculate_balance(node2);
#if 0
          if (node2->balance < -1)
          {
            rotate_left(tree, node2);
            node2 = node2->parent;
          }
          else if (node2->balance > 1)
          {
            rotate_right(tree, node2);
            node2 = node2->parent;
          }
#endif
          node2 = node2->parent;
        }
        avl_tree_insert_repair(tree, node);
        return;
      }
      node2 = node2->left;
    }
    else
    {
      if (node2->right == NULL)
      {
        //node2->balance++;
        node2->right = node;
        node->parent = node2;
        node2 = node2->parent;
        while (node2 != NULL)
        {
          //avl_calculate_balance(node2);
#if 0
          if (node2->balance < -1)
          {
            rotate_left(tree, node2);
            node2 = node2->parent;
          }
          else if (node2->balance > 1)
          {
            rotate_right(tree, node2);
            node2 = node2->parent;
          }
#endif
          node2 = node2->parent;
        }
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
  //return node->left == NULL && node->right == NULL;
}

static void __attribute__((unused)) avl_tree_exchange(struct avl_tree *tree, struct avl_tree_node *n1, struct avl_tree_node *n2)
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
  n1->balance = n2_balance;
  n2->balance = n1_balance;
}

static void __attribute__((unused)) avl_tree_replace(struct avl_tree *tree, struct avl_tree_node *n1, struct avl_tree_node *n2)
{
  struct avl_tree_node *n1_parent = n1->parent;
  struct avl_tree_node *n1_left = n1->left;
  struct avl_tree_node *n1_right = n1->right;
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

static void avl_tree_delete_repair(struct avl_tree *tree, struct avl_tree_node *N)
{
  struct avl_tree_node *X = NULL, *G = NULL, *Z = NULL;
  int b = 0;
  for (X = N->parent; X != NULL; X = G) { // Loop (possibly up to the root)
      //printf("iteration, balance %d\n", X->balance);
      G = X->parent; // Save parent of X around rotations
      // BalanceFactor(X) has not yet been updated!
      if (N == X->left) { // the left subtree decreases
          if (X->balance > 0) { // X is right-heavy
              // ===> the temporary BalanceFactor(X) == +2
              // ===> rebalancing is required.
              Z = X->right; // Sibling of N (higher by 2)
              b = Z->balance;
              if (b < 0)                     // Right Left Case     (see figure 5)
                  N = rotate_rightleft(X, Z); // Double rotation: Right(Z) then Left(X)
              else                           // Right Right Case    (see figure 4)
                  N = rotate_left(X, Z);     // Single rotation Left(X)
              // After rotation adapt parent link
          } else {
              if (X->balance == 0) {
                  X->balance = +1; // N’s height decrease is absorbed at X.
                  break; // Leave the loop
              }
              N = X;
              N->balance = 0; // Height(N) decreases by 1
              continue;
          }
      } else { // (N == right_child(X)): The right subtree decreases
          if (X->balance < 0) { // X is left-heavy
              // ===> the temporary BalanceFactor(X) == –2
              // ===> rebalancing is required.
              Z = X->left; // Sibling of N (higher by 2)
              b = Z->balance;
              if (b > 0)                     // Left Right Case
                  N = rotate_leftright(X, Z); // Double rotation: Left(Z) then Right(X)
              else                        // Left Left Case
                  N = rotate_right(X, Z);    // Single rotation Right(X)
              // After rotation adapt parent link
          } else {
              if (X->balance == 0) {
                  X->balance = -1; // N’s height decrease is absorbed at X.
                  break; // Leave the loop
              }
              N = X;
              N->balance = 0; // Height(N) decreases by 1
              continue;
          }
      }
      // After a rotation adapt parent link:
      // N is the new root of the rotated subtree
      N->parent = G;
      if (G != NULL) {
          if (X == G->left)
              G->left = N;
          else
              G->right = N;
          if (b == 0)
              break; // Height does not change: Leave the loop
      } else {
          tree->root = N; // N is the new root of the total tree
      }
      // Height(N) decreases by 1 (== old Height(X)-1)
  }
  // Unless loop is left via break, the height of the total tree decreases by 1.
}


static void avl_tree_delete_one_child(struct avl_tree *tree, struct avl_tree_node *node)
{
  /*
   * Precondition: n has at most one non-leaf child.
   */
  struct avl_tree_node *child = is_leaf(node->right) ? node->left : node->right;
  //avl_tree_replace(tree, node, child);
  avl_tree_delete_repair(tree, node);
  child = is_leaf(node->right) ? node->left : node->right;
#if 1
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
#endif
  if (child != NULL)
  {
    //avl_tree_delete_repair(tree, child);
    //child->is_black = node->is_black; // XXX
  }
}

void avl_tree_delete(struct avl_tree *tree, struct avl_tree_node *node)
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
    struct avl_tree_node *node2 = node->right;
    //struct avl_tree_node *oldright = node->right;
    for (;;)
    {
      if (node2->left == NULL)
      {
        break;
      }
      node2 = node2->left;
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
    //printf("exchange\n");
    avl_tree_exchange(tree, node, node2);
    //print_tree(tree);
    //abort();
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
    //node2->is_black = node->is_black; // XXX
    //avl_tree_replace(tree, node, node2);
    //node2->right = oldright;
    //oldright->parent = node2;
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

