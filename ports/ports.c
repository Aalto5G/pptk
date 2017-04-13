#include "ports.h"

void linkedlistfunc(struct packet *pkt, void *userdata)
{
  struct linkedlistfunc_userdata *ud = userdata;
  linked_list_add_tail(&pkt->node, ud->head);
}
