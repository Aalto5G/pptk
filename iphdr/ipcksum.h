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

void ip_cksum_feed(struct ip_cksum_ctx *ctx, const void *buf, size_t sz);

uint16_t ip_hdr_cksum_calc(const void *iphdr, uint16_t iplen);

uint16_t tcp_cksum_calc(
  const void *iphdr, uint16_t iplen, const void *tcphdr, uint16_t tcplen);

uint16_t udp_cksum_calc(
  const void *iphdr, uint16_t iplen, const void *udphdr, uint16_t udplen);

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

static inline void udp_set_cksum_calc(
  const void *iphdr, uint16_t iplen, void *udphdr, uint16_t udplen)
{
  if (iplen < 20)
  {
    abort();
  }
  if (udplen < 8)
  {
    abort();
  }
  udp_set_cksum(udphdr, 0);
  udp_set_cksum(udphdr, udp_cksum_calc(iphdr, iplen, udphdr, udplen));
}

#endif
