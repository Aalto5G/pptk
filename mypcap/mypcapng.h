#ifndef _MYPCAPNG_H_
#define _MYPCAPNG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "byteswap.h"
#include "hdr.h"
#include "dynarr.h"

struct pcapng_in_interface {
  char *name;
  uint32_t snaplen;
  uint16_t linktype;
  uint8_t tsresol;
  int64_t tsoffset;
};

struct pcapng_in_ctx {
  FILE *f;
  int swap;
  int ns;
  int32_t thiszone;
  DYNARR(struct pcapng_in_interface) interfaces;
};

void pcapng_in_ctx_free(struct pcapng_in_ctx *ctx);

int pcapng_in_ctx_read(
  struct pcapng_in_ctx *ctx, void **buf, size_t *bufcapacity,
  size_t *len, size_t *snap, uint64_t *time64, const char **ifname);

int pcapng_in_ctx_init(
  struct pcapng_in_ctx *ctx, const char *fname, int enforce_ethernet);

#endif
