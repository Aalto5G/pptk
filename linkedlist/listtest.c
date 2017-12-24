#include "linkedlist.h"
#include "containerof.h"
#include <stdio.h>
#include <stdlib.h>

struct entry {
  struct linked_list_node node;
  int i;
};

int main(int argc, char **argv)
{
  struct linked_list_head head;
  struct linked_list_node *node, *tmp;
  size_t i;
  struct entry entries[10];
  struct entry extra;
  linked_list_head_init(&head);
  for (i = 0; i < 10; i++)
  {
    if (linked_list_size(&head) != i)
    {
      abort();
    }
    linked_list_add_tail(&entries[i].node, &head);
    entries[i].i = i;
    LINKED_LIST_FOR_EACH(node, &head)
    {
      printf("%d ", CONTAINER_OF(node, struct entry, node)->i);
    }
    printf("\n=\n");
    LINKED_LIST_FOR_EACH_REVERSE(node, &head)
    {
      printf("%d ", CONTAINER_OF(node, struct entry, node)->i);
    }
    printf("\n--\n");
  }
  LINKED_LIST_FOR_EACH_SAFE(node, tmp, &head)
  {
    linked_list_delete(node);
  }
  for (i = 0; i < 10; i++)
  {
    if (linked_list_size(&head) != i)
    {
      abort();
    }
    linked_list_add_head(&entries[i].node, &head);
    entries[i].i = i;
    LINKED_LIST_FOR_EACH(node, &head)
    {
      printf("%d ", CONTAINER_OF(node, struct entry, node)->i);
    }
    printf("\n=\n");
    LINKED_LIST_FOR_EACH_REVERSE(node, &head)
    {
      printf("%d ", CONTAINER_OF(node, struct entry, node)->i);
    }
    printf("\n--\n");
  }
  LINKED_LIST_FOR_EACH_REVERSE_SAFE(node, tmp, &head)
  {
    linked_list_delete(node);
  }

  for (i = 0; i < 10; i++)
  {
    linked_list_add_tail(&entries[i].node, &head);
    entries[i].i = i;
  }
  linked_list_add_before(&extra.node, &entries[5].node);
  extra.i = 1000;
  LINKED_LIST_FOR_EACH(node, &head)
  {
    printf("%d ", CONTAINER_OF(node, struct entry, node)->i);
  }
  printf("\n=\n");
  LINKED_LIST_FOR_EACH_REVERSE(node, &head)
  {
    printf("%d ", CONTAINER_OF(node, struct entry, node)->i);
  }
  printf("\n--\n");
  LINKED_LIST_FOR_EACH_REVERSE_SAFE(node, tmp, &head)
  {
    linked_list_delete(node);
  }

  for (i = 0; i < 10; i++)
  {
    linked_list_add_tail(&entries[i].node, &head);
    entries[i].i = i;
  }
  linked_list_add_after(&extra.node, &entries[5].node);
  extra.i = 1000;
  LINKED_LIST_FOR_EACH(node, &head)
  {
    printf("%d ", CONTAINER_OF(node, struct entry, node)->i);
  }
  printf("\n=\n");
  LINKED_LIST_FOR_EACH_REVERSE(node, &head)
  {
    printf("%d ", CONTAINER_OF(node, struct entry, node)->i);
  }
  printf("\n--\n");
  LINKED_LIST_FOR_EACH_REVERSE_SAFE(node, tmp, &head)
  {
    linked_list_delete(node);
  }

  return 0;
}
