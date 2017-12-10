#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"
#include "containerof.h"

struct test_entry {
  struct rb_tree_node node;
  int i;
};

static int cmp(struct rb_tree_node *n1, struct rb_tree_node *n2, void *userdata)
{
  struct test_entry *e1 = CONTAINER_OF(n1, struct test_entry, node);
  struct test_entry *e2 = CONTAINER_OF(n2, struct test_entry, node);
  if (e1->i > e2->i)
  {
    return 1;
  }
  if (e1->i < e2->i)
  {
    return -1;
  }
  return 0;
}

static void insert_random(struct rb_tree *tree)
{
  struct test_entry *e = malloc(sizeof(*e));
  e->i = rand();
  printf("inserting %d\n", e->i);
  rb_tree_insert(tree, &e->node);
}

static void print_tree(struct rb_tree *tree);

static void delete_random(struct rb_tree *tree)
{
  struct rb_tree_node *node = tree->root;
  if (node == NULL)
  {
    return;
  }
  for (;;)
  {
    int tri = rand()%3;
    if (tri == 0)
    {
      printf("deleting %d\n", CONTAINER_OF(node, struct test_entry, node)->i);
      rb_tree_delete(tree, node);
      free(CONTAINER_OF(node, struct test_entry, node));
      return;
    }
    else if (tri == 1)
    {
      if (node->left == NULL)
      {
        printf("deleting %d\n", CONTAINER_OF(node, struct test_entry, node)->i);
        rb_tree_delete(tree, node);
        free(CONTAINER_OF(node, struct test_entry, node));
        return;
      }
      node = node->left;
    }
    else
    {
      if (node->right == NULL)
      {
        printf("deleting %d\n", CONTAINER_OF(node, struct test_entry, node)->i);
        rb_tree_delete(tree, node);
        free(CONTAINER_OF(node, struct test_entry, node));
        return;
      }
      node = node->right;
    }
  }
}

static void indent(int level)
{
  int i;
  for (i = 0; i < level; i++)
  {
    printf(" ");
  }
}

static void print_node(int level, struct rb_tree_node *node)
{
  struct test_entry *e = CONTAINER_OF(node, struct test_entry, node);
  indent(level);
  printf("is_black: %d\n", node->is_black);
  indent(level);
  printf("i: %d\n", e->i);
  indent(level);
  printf("left:\n");
  if (node->left != NULL)
  {
    print_node(level+2, node->left);
  }
  indent(level);
  printf("right:\n");
  if (node->right != NULL)
  {
    print_node(level+2, node->right);
  }
}

static void print_tree(struct rb_tree *tree)
{
  if (tree->root != NULL)
  {
    print_node(0, tree->root);
  }
}

int main(int argc, char **argv)
{
  struct rb_tree tree = {};
  int i;
  tree.cmp = cmp;
  for (i = 0; i < 1000*1000; i++)
  {
    printf("i = %d\n", i);
    //print_tree(&tree);
    if (rand() % 2)
    {
      delete_random(&tree);
    }
    else
    {
      insert_random(&tree);
    }
    if (!rb_tree_valid(&tree))
    {
      printf("invalid\n");
      print_tree(&tree);
      abort();
    }
  }
}
