#ifndef _IPFRAG_H_
#define _IPFRAG_H_
#include "packet.h"
#include "asalloc.h"

struct fragment {
  uint16_t datastart;
  uint16_t datalen;
  struct packet *pkt;
};

int fragment4(struct as_alloc_local *loc, const void *pkt, uint16_t sz,
              struct fragment *frags, size_t fragnum);

#endif
