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

static inline uint16_t ip_update_cksum16(
  uint16_t old_cksum16, uint16_t old16, uint16_t new16)
{
  uint32_t old_cksum = old_cksum16;
  old_cksum = (uint16_t)~old_cksum;
  old_cksum += old16;
  old_cksum += new16;
  while (old_cksum >> 16)
  {
    old_cksum = (old_cksum & 0xffff) + (old_cksum >> 16);
  }
  old_cksum = (uint16_t)~old_cksum;
  return old_cksum;
}

static inline uint32_t ip_update_cksum32(
  uint16_t old_cksum, uint32_t old32, uint32_t new32)
{
  uint16_t new1 = (new32>>16), old1 = ~(old32>>16);
  uint16_t new2 = (new32&0xFFFF), old2 = ~(old32&0xFFFF);
  uint16_t x;
  x = ip_update_cksum16(ip_update_cksum16(old_cksum, old1, new1), old2, new2);
  return x;
}

static inline void ip_set_src_cksum_update(
  void *iphdr, uint16_t iplen, uint8_t proto, void *payhdr, uint16_t paylen, uint32_t src)
{
  uint32_t old_src = ip_src(iphdr);
  uint16_t old_cksum = ip_hdr_cksum(iphdr);
  old_cksum = ip_update_cksum32(old_cksum, old_src, src);
  ip_set_hdr_cksum(iphdr, old_cksum);
  if (proto == 6)
  {
    old_cksum = tcp_cksum(payhdr);
    old_cksum = ip_update_cksum32(old_cksum, old_src, src);
    tcp_set_cksum(payhdr, old_cksum);
  }
  else if (proto == 17)
  {
    old_cksum = udp_cksum(payhdr);
    old_cksum = ip_update_cksum32(old_cksum, old_src, src);
    udp_set_cksum(payhdr, old_cksum);
  }
  ip_set_src(iphdr, src);
}

static inline void ip_set_dst_cksum_update(
  void *iphdr, uint16_t iplen, uint8_t proto, void *payhdr, uint16_t paylen, uint32_t dst)
{
  uint32_t old_dst = ip_dst(iphdr);
  uint16_t old_cksum = ip_hdr_cksum(iphdr);
  old_cksum = ip_update_cksum32(old_cksum, old_dst, dst);
  ip_set_hdr_cksum(iphdr, old_cksum);
  if (proto == 6)
  {
    old_cksum = tcp_cksum(payhdr);
    old_cksum = ip_update_cksum32(old_cksum, old_dst, dst);
    tcp_set_cksum(payhdr, old_cksum);
  }
  else if (proto == 17)
  {
    old_cksum = udp_cksum(payhdr);
    old_cksum = ip_update_cksum32(old_cksum, old_dst, dst);
    udp_set_cksum(payhdr, old_cksum);
  }
  ip_set_dst(iphdr, dst);
}

static inline int ip_decr_ttl_cksum_update(void *pkt)
{
  uint8_t ttl;
  uint8_t proto;
  uint16_t whole_field_old, whole_field_new;
  uint16_t old_cksum = ip_hdr_cksum(pkt);
  ttl = ip_ttl(pkt);
  proto = ip_proto(pkt);
  if (ttl == 0)
  {
    abort();
  }
  whole_field_old = (ttl<<8)|proto;
  ttl--;
  whole_field_new = (ttl<<8)|proto;
  old_cksum = ip_update_cksum32(old_cksum, whole_field_old, whole_field_new);
  ip_set_hdr_cksum(pkt, old_cksum);
  ip_set_ttl(pkt, ttl);
  return ttl > 0;
}

#endif
