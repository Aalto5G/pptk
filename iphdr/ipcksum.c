#include "ipcksum.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hdr.h"
#include "iphdr.h"

void ip_cksum_feed(struct ip_cksum_ctx *ctx, const void *buf, size_t sz)
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

uint16_t ip_hdr_cksum_calc(const void *iphdr, uint16_t iplen)
{
  uint8_t ihl = ip_hdr_len(iphdr);
  struct ip_cksum_ctx ctx = IP_CKSUM_CTX_INITER;
  if (ihl > iplen)
  {
    abort();
  }
  ip_cksum_feed(&ctx, iphdr, ihl);
  return ip_cksum_postprocess(&ctx);
}

uint16_t tcp_cksum_calc(
  const void *iphdr, uint16_t iplen, const void *tcphdr, uint16_t tcplen)
{
  struct ip_cksum_ctx ctx = IP_CKSUM_CTX_INITER;
  uint32_t u32;
  if (iplen < 20)
  {
    abort();
  }
  u32 = htonl(ip_src(iphdr));
  ip_cksum_feed32ptr(&ctx, &u32);
  u32 = htonl(ip_dst(iphdr));
  ip_cksum_feed32ptr(&ctx, &u32);
  ip_cksum_add16(&ctx, htons(6));
  ip_cksum_add16(&ctx, htons(tcplen));
  ip_cksum_feed(&ctx, tcphdr, tcplen);
  return ip_cksum_postprocess(&ctx);
}

uint16_t udp_cksum_calc(
  const void *iphdr, uint16_t iplen, const void *udphdr, uint16_t udplen)
{
  struct ip_cksum_ctx ctx = IP_CKSUM_CTX_INITER;
  uint32_t u32;
  if (iplen < 20)
  {
    abort();
  }
  u32 = htonl(ip_src(iphdr));
  ip_cksum_feed32ptr(&ctx, &u32);
  u32 = htonl(ip_dst(iphdr));
  ip_cksum_feed32ptr(&ctx, &u32);
  ip_cksum_add16(&ctx, htons(17));
  ip_cksum_add16(&ctx, htons(udplen));
  ip_cksum_feed(&ctx, udphdr, udplen);
  return ip_cksum_postprocess(&ctx);
}
