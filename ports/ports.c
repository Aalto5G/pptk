#include "ports.h"

void allocifdiscardfunc(struct packet *pkt, void *userdata)
{
  struct allocifdiscardfunc_userdata *ud = userdata;
  allocif_free(ud->intf, pkt);
}

void linkedlistfunc(struct packet *pkt, void *userdata)
{
  struct linkedlistfunc_userdata *ud = userdata;
  linked_list_add_tail(&pkt->node, ud->head);
}
