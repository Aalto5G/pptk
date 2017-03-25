#ifndef _PACKET_H_
#define _PACKET_H_

enum packet_direction {
  PACKET_DIRECTION_UPLINK = 0,
  PACKET_DIRECTION_DOWNLINK = 0,
};

struct packet {
  struct linked_list_node node;
  enum packet_direction direction;
  size_t sz;
  // after this: packet data
};

void *packet_data(struct packet *pkt)
{
  return ((char*)pkt) + sizeof(struct packet);
}

const void *packet_const_data(const struct packet *pkt)
{
  return ((const char*)pkt) + sizeof(struct packet);
}

void *packet_size(size_t sz)
{
  return sz + sizeof(struct packet);
}

#endif