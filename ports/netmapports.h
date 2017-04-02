#ifndef _NETMAPPORTS_H_
#define _NETMAPPORTS_H_

#include "packet.h"
#include "asalloc.h"
#include "ports.h"

void netmapfunc(struct packet *pkt, void *userdata);

struct netmapfunc_userdata {
  struct as_alloc_local *loc;
  struct nm_desc *nmd;
};

#endif
