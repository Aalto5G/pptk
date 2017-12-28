#ifndef _MYPCAP_H_
#define _MYPCAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "byteswap.h"
#include "hdr.h"

struct pcap_in_ctx {
  FILE *f;
  int swap;
  int ns;
  int32_t thiszone;
};

struct pcap_out_ctx {
  FILE *f;
};

void pcap_in_ctx_free(struct pcap_in_ctx *ctx);

void pcap_out_ctx_free(struct pcap_out_ctx *ctx);

int pcap_in_ctx_read(
  struct pcap_in_ctx *ctx, void **buf, size_t *bufcapacity,
  size_t *len, size_t *snap, uint64_t *time64);

int pcap_out_ctx_init(
  struct pcap_out_ctx *ctx, const char *fname);

int pcap_out_ctx_write(
  struct pcap_out_ctx *ctx, void *buf, size_t size, uint64_t time64);

int pcap_in_ctx_init_file(
  struct pcap_in_ctx *ctx, FILE *f, int enforce_ethernet);

int pcap_in_ctx_init(
  struct pcap_in_ctx *ctx, const char *fname, int enforce_ethernet);

#endif
