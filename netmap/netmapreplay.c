#define _GNU_SOURCE
#define NETMAP_WITH_LIBS
#include <pthread.h>
#include "dynarr.h"
#include "arp.h"
#include "iphdr.h"
#include "ipcksum.h"
#include "packet.h"
#include "net/netmap_user.h"
#include "mypcap.h"
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

struct pkt {
  int len;
  char data[1514];
};

int main(int argc, char **argv)
{
  struct nmreq nmr;
  void *buf = NULL;
  size_t bufcapacity = 0;
  size_t len = 0;
  size_t snap = 0;
  int sz;
  DYNARR(struct pkt*) ar = DYNARR_INITER;
  struct pcap_in_ctx inctx;
  uint64_t bytes = 0, pkts = 0;
  uint64_t last_bytes = 0, last_pkts = 0;
  uint64_t last_time64 = 0;

  setlinebuf(stdout);

  if (argc != 3)
  {
    printf("usage: %s vale0:1 file.pcap\n", argv[0]);
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

  if (pcap_in_ctx_init(&inctx, argv[2], 1) != 0)
  {
    printf("cannot open %s\n", argv[2]);
    exit(1);
  }

  for (;;)
  {
    int ret;
    struct pkt *pkt;
    uint64_t time64;
    ret = pcap_in_ctx_read(&inctx, &buf, &bufcapacity, &len, &snap, &time64);
    if (ret == 0)
    {
      break;
    }
    else if (ret != 1)
    {
      printf("can't read pcap\n");
      exit(1);
    }
    if (len != snap)
    {
      printf("truncated pcap\n");
      exit(1);
    }
    if (len > sizeof(pkt->data))
    {
      printf("overlong packet\n");
      exit(1);
    }
    pkt = malloc(sizeof(*pkt));
    if (pkt == NULL)
    {
      printf("out of memory\n");
      exit(1);
    }
    memcpy(pkt->data, buf, len);
    pkt->len = len;
    if (!DYNARR_PUSH_BACK(&ar, pkt))
    {
      printf("out of memory\n");
      exit(1);
    }
  }
  sz = DYNARR_SIZE(&ar);
  last_time64 = gettime64();
  for (;;)
  {
    for (int i = 0; i < sz; i++)
    {
      nm_my_inject(dlnmd, DYNARR_GET(&ar, i)->data, DYNARR_GET(&ar, i)->len);
      pkts++;
      bytes += DYNARR_GET(&ar, i)->len;
      if (pkts % 16 == 0 && gettime64() >= last_time64 + 1000*1000)
      {
        uint64_t time64 = gettime64();
        uint64_t pdiff = pkts - last_pkts;
        uint64_t bdiff = bytes - last_bytes;
        double tdiff = time64 - last_time64;
        printf("%g MPPS %g Gbps\n", pdiff/tdiff, bdiff*8/1000/tdiff);
        last_pkts = pkts;
        last_bytes = bytes;
        last_time64 = time64;
      }
    }
  }
  return 0;
}
