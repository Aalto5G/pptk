#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "byteswap.h"
#include "hdr.h"
#include "mypcapjoker.h"

void pcap_joker_ctx_free(struct pcap_joker_ctx *ctx)
{
  if (ctx->is_ng)
  {
    pcapng_in_ctx_free(&ctx->u.ng);
  }
  else
  {
    pcap_in_ctx_free(&ctx->u.old);
  }
}

int pcap_joker_ctx_read(
  struct pcap_joker_ctx *ctx, void **buf, size_t *bufcapacity,
  size_t *len, size_t *snap, uint64_t *time64, const char **ifname)
{
  if (ctx->is_ng)
  {
    return pcapng_in_ctx_read(&ctx->u.ng, buf, bufcapacity, len, snap, time64, ifname);
  }
  else
  {
    int ret;
    ret = pcap_in_ctx_read(&ctx->u.old, buf, bufcapacity, len, snap, time64);
    if (ifname)
    {
      *ifname = ctx->ifname;
    }
    return ret;
  }
}

int pcap_joker_ctx_init_file(
  struct pcap_joker_ctx *ctx, FILE *f, int enforce_ethernet, const char *ifname)
{
  int ch = fgetc(f);
  if (ungetc(ch, f) != ch)
  {
    abort();
  }
  ctx->ifname = ifname;
  if (ch == '\x0a')
  {
    ctx->is_ng = 1;
    return pcapng_in_ctx_init_file(&ctx->u.ng, f, enforce_ethernet);
  }
  else
  {
    ctx->is_ng = 0;
    return pcap_in_ctx_init_file(&ctx->u.old, f, enforce_ethernet);
  }
}

int pcap_joker_ctx_init(
  struct pcap_joker_ctx *ctx, const char *fname, int enforce_ethernet,
  const char *ifname)
{
  FILE *f = fopen(fname, "rb");
  return pcap_joker_ctx_init_file(ctx, f, enforce_ethernet, ifname);
}
