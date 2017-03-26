#ifndef _PORTS_H_
#define _PORTS_H_

#include "packet.h"

struct port {
  void (*portfunc)(struct packet *pkt, void *userdata);
  void *userdata;
};

#endif
