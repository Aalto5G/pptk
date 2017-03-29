#ifndef _PORTLIST_H_
#define _PORTLIST_H_

#include "containerof.h"
#include "linkedlist.h"

struct port_list_entry {
  struct linked_list_node node;
  uint16_t port;
};

struct port_list {
  struct linked_list_head list;
};

static inline void port_list_init(struct port_list *list)
{
  linked_list_head_init(&list->list);
}

static inline void port_list_add(struct port_list *list, uint16_t x)
{
  struct port_list_entry *e = malloc(sizeof(*e));
  if (e == NULL || x == 0)
  {
    abort();
  }
  e->port = x;
  linked_list_add_tail(&e->node, &list->list);
}

static inline uint16_t port_list_get(struct port_list *list)
{
  struct port_list_entry *e;
  uint16_t result;
  if (list->list.node.next == NULL)
  {
    return 0;
  }
  e = CONTAINER_OF(list->list.node.next, struct port_list_entry, node);
  result = e->port;
  linked_list_delete(&e->node);
  free(e);
  return result;
}

#endif
