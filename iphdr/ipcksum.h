#ifndef _IPCKSUM_H_
#define _IPCKSUM_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hdr.h"
#include "iphdr.h"

struct ip_cksum_ctx {
  uint32_t sum;
};

#define IP_CKSUM_CTX_INITER { .sum = 0 }

static inline uint16_t ip_cksum_postprocess(struct ip_cksum_ctx *ctx)
{
  uint32_t sum = ctx->sum;
  while (sum>>16)
  {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  return htons(~sum);
}

static inline void ip_cksum_add16(struct ip_cksum_ctx *ctx, uint16_t val16)
{
  ctx->sum += val16;
}

static inline void ip_cksum_add_leftover(struct ip_cksum_ctx *ctx, uint8_t val)
{
  ctx->sum += val;
}

static inline void ip_cksum_feed32ptr(struct ip_cksum_ctx *ctx, const void *buf)
{
  const char *cbuf = buf;
  ip_cksum_add16(ctx, hdr_get16h(&cbuf[0]));
  ip_cksum_add16(ctx, hdr_get16h(&cbuf[2]));
}

static inline void ip_cksum_feed(struct ip_cksum_ctx *ctx, const void *buf, size_t sz)
{
  const char *cbuf = buf;
  struct ip_cksum_ctx ctx2 = *ctx;
  while (sz >= 16)
  {
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[0]));
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[2]));
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[4]));
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[6]));
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[8]));
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[10]));
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[12]));
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[14]));
    sz -= 16;
    cbuf += 16;
  }
  while (sz >= 2)
  {
    ip_cksum_add16(&ctx2, hdr_get16h(&cbuf[0]));
    sz -= 2;
    cbuf += 2;
  }
  if (sz >= 1)
  {
    ip_cksum_add_leftover(&ctx2, hdr_get8h(&cbuf[0]));
  }
  *ctx = ctx2;
}

uint16_t ip_hdr_cksum_calc(const void *iphdr, uint16_t iplen);

uint16_t tcp_cksum_calc(
  const void *iphdr, uint16_t iplen, const void *tcphdr, uint16_t tcplen);

static inline void ip_set_hdr_cksum_calc(void *iphdr, uint16_t iplen)
{
  uint16_t cksum;
  if (iplen < 20)
  {
    abort();
  }
  ip_set_hdr_cksum(iphdr, 0);
  cksum = ip_hdr_cksum_calc(iphdr, iplen);
  ip_set_hdr_cksum(iphdr, cksum);
}

static inline void tcp_set_cksum_calc(
  const void *iphdr, uint16_t iplen, void *tcphdr, uint16_t tcplen)
{
  if (iplen < 20)
  {
    abort();
  }
  if (tcplen < 20)
  {
    abort();
  }
  tcp_set_cksum(tcphdr, 0);
  tcp_set_cksum(tcphdr, tcp_cksum_calc(iphdr, iplen, tcphdr, tcplen));
}

#endif
