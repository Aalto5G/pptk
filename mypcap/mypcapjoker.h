#ifndef _MYPCAPJOKER_H_
#define _MYPCAPJOKER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "byteswap.h"
#include "hdr.h"
#include "mypcap.h"
#include "mypcapng.h"

struct pcap_joker_ctx {
  int is_ng;
  const char *ifname;
  union {
    struct pcap_in_ctx old;
    struct pcapng_in_ctx ng;
  } u;
};

void pcap_joker_ctx_free(struct pcap_joker_ctx *ctx);

int pcap_joker_ctx_read(
  struct pcap_joker_ctx *ctx, void **buf, size_t *bufcapacity,
  size_t *len, size_t *snap, uint64_t *time64, const char **ifname);

int pcap_joker_ctx_init_file(
  struct pcap_joker_ctx *ctx, FILE *f, int enforce_ethernet,
  const char *ifname);

int pcap_joker_ctx_init(
  struct pcap_joker_ctx *ctx, const char *fname, int enforce_ethernet,
  const char *ifname);

#endif
