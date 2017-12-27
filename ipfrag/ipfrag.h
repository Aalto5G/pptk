#ifndef _IPFRAG_H_
#define _IPFRAG_H_
#include "packet.h"
#include "allocif.h"

struct fragment {
  uint16_t datastart;
  uint16_t datalen;
  struct packet *pkt;
};

int fragment4(struct allocif *loc, const void *pkt, uint16_t sz,
              struct fragment *frags, size_t fragnum);

#endif
