#ifndef _PORTS_H_
#define _PORTS_H_

#include "packet.h"
#include "linkedlist.h"

struct port {
  void (*portfunc)(struct packet *pkt, void *userdata);
  void *userdata;
};

struct linkedlistfunc_userdata {
  struct linked_list_head *head;
};

void linkedlistfunc(struct packet *pkt, void *userdata);

#endif
