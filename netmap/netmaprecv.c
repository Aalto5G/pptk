#define _GNU_SOURCE
#define NETMAP_WITH_LIBS
#include <pthread.h>
#include "arp.h"
#include "iphdr.h"
#include "ipcksum.h"
#include "packet.h"
#include "net/netmap_user.h"
#include <sys/poll.h>
#include "time64.h"

struct nm_desc *nmd;

int main(int argc, char **argv)
{
  uint64_t last_time64 = gettime64();
  uint64_t pkts = 0, bytes = 0;
  uint64_t last_pkts = 0, last_bytes = 0;

  setlinebuf(stdout);

  if (argc != 2)
  {
    printf("usage: %s vale1:0\n", argv[0]);
    exit(1);
  }

  nmd = nm_open(argv[1], NULL, 0, NULL);
  if (nmd == NULL)
  {
    printf("cannot open %s\n", argv[1]);
    exit(1);
  }

  last_time64 = gettime64();
  for (;;)
  {
    int i;
    struct pollfd pfds[1];
    uint64_t time64;
    pfds[0].fd = nmd->fd;
    pfds[0].events = POLLIN;
    poll(pfds, 1, 5);
    time64 = gettime64();
    if (time64 - last_time64 > 1000*1000)
    {
      uint64_t pdiff = pkts - last_pkts;
      uint64_t bdiff = bytes - last_bytes;
      double tdiff = time64 - last_time64;
      printf("%g MPPS %g Gbps\n",
             pdiff/tdiff, bdiff*8/1000/tdiff);
      last_time64 = time64;
      last_pkts = pkts;
      last_bytes = bytes;
    }
    for (i = 0; i < 1000; i++)
    {
      struct nm_pkthdr hdr;
      unsigned char *pkt;
      pkt = nm_nextpkt(nmd, &hdr);
      if (pkt == NULL)
      {
        break;
      }
      pkts += 1;
      bytes += hdr.len;
    }
  }
  return 0;
}
