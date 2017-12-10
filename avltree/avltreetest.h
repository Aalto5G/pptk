#ifndef _AVL_TREE_TEST_
#define _AVL_TREE_TEST_

struct test_entry {
  struct avl_tree_node node;
  int i;
};

void print_tree(struct avl_tree *tree);

#endif
