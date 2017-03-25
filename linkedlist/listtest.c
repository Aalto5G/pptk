#include "linkedlist.h"
#include "containerof.h"
#include <stdio.h>

struct entry {
  struct linked_list_node node;
  int i;
};

int main(int argc, char **argv)
{
  struct linked_list_head head;
  struct linked_list_node *node, *tmp;
  int i;
  struct entry entries[10];
  linked_list_head_init(&head);
  for (i = 0; i < 10; i++)
  {
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
  return 0;
}
