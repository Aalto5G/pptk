#define _GNU_SOURCE
#define NETMAP_WITH_LIBS
#include <pthread.h>
#include "dynarr.h"
#include "iphdr.h"
#include "ipcksum.h"
#include "packet.h"
#include "net/netmap_user.h"
#include "mypcapjoker.h"
#include <sys/poll.h>
#include "time64.h"
#include "netmapcommon.h"

struct nm_desc *dlnmd, *ulnmd;

struct pkt {
  size_t len;
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
  struct pcap_joker_ctx inctx;
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

  if (pcap_joker_ctx_init(&inctx, argv[2], 1, "eth0") != 0)
  {
    printf("cannot open %s\n", argv[2]);
    exit(1);
  }

  for (;;)
  {
    int ret;
    struct pkt *pkt;
    uint64_t time64;
    ret = pcap_joker_ctx_read(&inctx, &buf, &bufcapacity, &len, &snap, &time64,
                              NULL);
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
