#include "packet.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
  struct packet *pkt = malloc(packet_size(1514));
  pkt->direction = PACKET_DIRECTION_UPLINK;
  pkt->sz = 1514;
  memset(packet_data(pkt), 0, 1514);
  free(pkt);
  return 0;
}
