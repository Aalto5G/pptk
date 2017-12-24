#ifndef _PACKET_H_
#define _PACKET_H_

#include "linkedlist.h"
#include <stddef.h>
#include <stdint.h>

enum packet_direction {
  PACKET_DIRECTION_UPLINK = 0,
  PACKET_DIRECTION_DOWNLINK = 1,
};

struct hole {
  struct linked_list_node node;
  uint16_t first;
  uint16_t last;
};

struct packet {
  struct linked_list_node node;
  struct hole hole;
  enum packet_direction direction;
  size_t sz;
  // after this: packet data
};

static inline void *packet_data(struct packet *pkt)
{
  return ((char*)pkt) + sizeof(struct packet);
}

static inline const void *packet_const_data(const struct packet *pkt)
{
  return ((const char*)pkt) + sizeof(struct packet);
}

static inline size_t packet_size(size_t sz)
{
  return sz + sizeof(struct packet);
}

#endif
