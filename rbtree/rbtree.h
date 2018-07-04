#ifndef _RBTREE_H_
#define _RBTREE_H_

struct rb_tree_node {
  int is_black;
  struct rb_tree_node *left;
  struct rb_tree_node *right;
  struct rb_tree_node *parent;
};

typedef int (*rb_tree_cmp)(struct rb_tree_node *a, struct rb_tree_node *b, void *ud);

struct rb_tree {
  struct rb_tree_node *root;
  rb_tree_cmp cmp;
  void *cmp_userdata;
};

static inline void rb_tree_init(struct rb_tree *tree, rb_tree_cmp cmp, void *cmp_userdata)
{
  tree->root = NULL;
  tree->cmp = cmp;
  tree->cmp_userdata = cmp_userdata;
}

int rb_tree_valid(struct rb_tree *tree);

struct rb_tree_node *rb_tree_leftmost(struct rb_tree *tree);

struct rb_tree_node *rb_tree_rightmost(struct rb_tree *tree);

void rb_tree_insert_repair(struct rb_tree *tree, struct rb_tree_node *node);

void rb_tree_insert(struct rb_tree *tree, struct rb_tree_node *node);

void rb_tree_delete(struct rb_tree *tree, struct rb_tree_node *node);

#endif
