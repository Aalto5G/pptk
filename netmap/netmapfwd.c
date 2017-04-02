#define _GNU_SOURCE
#define NETMAP_WITH_LIBS
#include <pthread.h>
#include "arp.h"
#include "iphdr.h"
#include "ipcksum.h"
#include "packet.h"
#include "net/netmap_user.h"
#include <sys/poll.h>

static inline uint64_t gettime64(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000UL*1000UL + tv.tv_usec;
}

struct nm_desc *dlnmd, *ulnmd;

static inline void nm_my_inject(struct nm_desc *nmd, void *data, size_t sz)
{
  int i, j;
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      if (nm_inject(nmd, data, sz) == 0)
      {
        struct pollfd pollfd;
        pollfd.fd = nmd->fd;
        pollfd.events = POLLOUT;
        poll(&pollfd, 1, 0);
      }
      else
      {
        return;
      }
    }
    ioctl(nmd->fd, NIOCTXSYNC, NULL);
  }
}

int main(int argc, char **argv)
{
  uint64_t last_time64 = gettime64();
  uint64_t ulpkts = 0, ulbytes = 0, dlpkts = 0, dlbytes = 0;
  uint64_t last_ulpkts = 0, last_ulbytes = 0, last_dlpkts = 0, last_dlbytes = 0;
  struct nmreq nmr;

  setlinebuf(stdout);

  if (argc != 3)
  {
    printf("usage: %s vale0:1 vale1:1\n", argv[0]);
    exit(1);
  }

  memset(&nmr, 0, sizeof(nmr));
  nmr.nr_rx_slots = 256;
  nmr.nr_tx_slots = 64;
  dlnmd = nm_open(argv[1], &nmr, 0, NULL);
  if (dlnmd == NULL)
  {
    printf("cannot open %s\n", argv[1]);
    exit(1);
  }
  memset(&nmr, 0, sizeof(nmr));
  nmr.nr_rx_slots = 256;
  nmr.nr_tx_slots = 64;
  ulnmd = nm_open(argv[2], &nmr, 0, NULL);
  if (ulnmd == NULL)
  {
    printf("cannot open %s\n", argv[1]);
    exit(1);
  }

  last_time64 = gettime64();
  for (;;)
  {
    int i;
    struct pollfd pfds[2];
    uint64_t time64;
    pfds[0].fd = dlnmd->fd;
    pfds[0].events = POLLIN;
    pfds[1].fd = ulnmd->fd;
    pfds[1].events = POLLIN;
    poll(pfds, 2, 5);
    time64 = gettime64();
    if (time64 - last_time64 > 1000*1000)
    {
      uint64_t ulpdiff = ulpkts - last_ulpkts;
      uint64_t dlpdiff = dlpkts - last_dlpkts;
      uint64_t ulbdiff = ulbytes - last_ulbytes;
      uint64_t dlbdiff = dlbytes - last_dlbytes;
      double tdiff = time64 - last_time64;
      printf("%g MPPS %g Gbps ul, %g MPPS %g Gbps dl\n",
             ulpdiff/tdiff, ulbdiff*8/1000/tdiff,
             dlpdiff/tdiff, dlbdiff*8/1000/tdiff);
      last_time64 = time64;
      last_ulpkts = ulpkts;
      last_dlpkts = dlpkts;
      last_ulbytes = ulbytes;
      last_dlbytes = dlbytes;
    }
    for (i = 0; i < 1000; i++)
    {
      struct nm_pkthdr hdr;
      unsigned char *pkt;
      pkt = nm_nextpkt(dlnmd, &hdr);
      if (pkt == NULL)
      {
        break;
      }
      ulpkts += 1;
      ulbytes += hdr.len;
      nm_my_inject(ulnmd, pkt, hdr.len);
    }
    for (i = 0; i < 1000; i++)
    {
      struct nm_pkthdr hdr;
      unsigned char *pkt;
      pkt = nm_nextpkt(ulnmd, &hdr);
      if (pkt == NULL)
      {
        break;
      }
      dlpkts += 1;
      dlbytes += hdr.len;
      nm_my_inject(dlnmd, pkt, hdr.len);
    }
  }
  return 0;
}
